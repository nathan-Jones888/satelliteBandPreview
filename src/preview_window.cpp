#include "preview_window.h"

#include "demo_image.h"
#include "histogram_widget.h"
#include "image_canvas.h"

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGraphicsPixmapItem>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

namespace
{
constexpr qint64 kMaximumPreviewPixels = 40'000'000;
}

PreviewWindow::PreviewWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("高分影像波段组合与拉伸预览"));
    resize(1380, 860);
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f2f4f7; }"
        "QToolBar { spacing: 6px; padding: 5px; }"
        "QComboBox, QSpinBox, QPushButton { padding: 4px 7px; }"));

    auto* toolbar = addToolBar(QStringLiteral("影像工具"));
    toolbar->setMovable(false);
    QAction* openAction = toolbar->addAction(QStringLiteral("打开影像"));
    QAction* demoAction = toolbar->addAction(QStringLiteral("合成演示图"));
    QAction* saveAction = toolbar->addAction(QStringLiteral("导出 PNG"));
    toolbar->addSeparator();
    QAction* fitAction = toolbar->addAction(QStringLiteral("适合窗口"));
    QAction* zoomInAction = toolbar->addAction(QStringLiteral("放大"));
    QAction* zoomOutAction = toolbar->addAction(QStringLiteral("缩小"));

    connect(openAction, &QAction::triggered,
            this, &PreviewWindow::openImage);
    connect(demoAction, &QAction::triggered,
            this, &PreviewWindow::showDemoImage);
    connect(saveAction, &QAction::triggered,
            this, &PreviewWindow::savePreview);
    connect(fitAction, &QAction::triggered,
            this, [this]() { m_canvas->fitImage(); });
    connect(zoomInAction, &QAction::triggered, this, [this]() {
        m_canvas->zoomBy(1.25);
    });
    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        m_canvas->zoomBy(0.8);
    });

    m_canvas = new ImageCanvas(this);
    setCentralWidget(m_canvas);
    connect(m_canvas, &ImageCanvas::imagePixelHovered,
            this, &PreviewWindow::updatePixelReadout);

    auto* panel = new QWidget(this);
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    panelLayout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("显示参数"));
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    panelLayout->addWidget(title);

    auto* bandForm = new QFormLayout;
    m_redChannel = new QComboBox(panel);
    m_greenChannel = new QComboBox(panel);
    m_blueChannel = new QComboBox(panel);
    const auto populateChannels = [](QComboBox* combo) {
        combo->addItem(QStringLiteral("输入红通道"), 0);
        combo->addItem(QStringLiteral("输入绿通道"), 1);
        combo->addItem(QStringLiteral("输入蓝通道"), 2);
    };
    populateChannels(m_redChannel);
    populateChannels(m_greenChannel);
    populateChannels(m_blueChannel);
    bandForm->addRow(QStringLiteral("显示 R"), m_redChannel);
    bandForm->addRow(QStringLiteral("显示 G"), m_greenChannel);
    bandForm->addRow(QStringLiteral("显示 B"), m_blueChannel);
    panelLayout->addLayout(bandForm);

    auto* stretchTitle = new QLabel(QStringLiteral("百分位拉伸"));
    stretchTitle->setStyleSheet(
        QStringLiteral("font-size: 14px; font-weight: 600;"));
    panelLayout->addWidget(stretchTitle);

    auto* stretchForm = new QFormLayout;
    m_lowClip = new QSpinBox(panel);
    m_lowClip->setRange(0, 20);
    m_lowClip->setSuffix(QStringLiteral("%"));
    m_lowClip->setValue(2);
    m_highClip = new QSpinBox(panel);
    m_highClip->setRange(0, 20);
    m_highClip->setSuffix(QStringLiteral("%"));
    m_highClip->setValue(2);
    stretchForm->addRow(QStringLiteral("低端裁剪"), m_lowClip);
    stretchForm->addRow(QStringLiteral("高端裁剪"), m_highClip);
    panelLayout->addLayout(stretchForm);

    m_histogram = new HistogramWidget(panel);
    panelLayout->addWidget(m_histogram);

    m_imageInfo = new QLabel(QStringLiteral("尚未载入影像"));
    m_imageInfo->setWordWrap(true);
    m_imageInfo->setStyleSheet(
        QStringLiteral("color: #475467; background: #f9fafb; padding: 8px;"));
    panelLayout->addWidget(m_imageInfo);

    auto* resetButton = new QPushButton(
        QStringLiteral("恢复 RGB 默认组合"), panel);
    panelLayout->addWidget(resetButton);
    panelLayout->addStretch();

    auto* dock = new QDockWidget(QStringLiteral("波段与统计"), this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setWidget(panel);
    dock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(m_redChannel, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PreviewWindow::refreshPreview);
    connect(m_greenChannel, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PreviewWindow::refreshPreview);
    connect(m_blueChannel, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PreviewWindow::refreshPreview);
    connect(m_lowClip, qOverload<int>(&QSpinBox::valueChanged),
            this, &PreviewWindow::refreshPreview);
    connect(m_highClip, qOverload<int>(&QSpinBox::valueChanged),
            this, &PreviewWindow::refreshPreview);
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        const QSignalBlocker blockRed(m_redChannel);
        const QSignalBlocker blockGreen(m_greenChannel);
        const QSignalBlocker blockBlue(m_blueChannel);
        const QSignalBlocker blockLow(m_lowClip);
        const QSignalBlocker blockHigh(m_highClip);
        m_redChannel->setCurrentIndex(0);
        m_greenChannel->setCurrentIndex(1);
        m_blueChannel->setCurrentIndex(2);
        m_lowClip->setValue(2);
        m_highClip->setValue(2);
        refreshPreview();
    });

    m_statusInfo = new QLabel(QStringLiteral("PNG / JPEG 本地快视工具"));
    m_pixelInfo = new QLabel(QStringLiteral("像素：—"));
    statusBar()->addWidget(m_statusInfo, 1);
    statusBar()->addPermanentWidget(m_pixelInfo);

    showDemoImage();
}

bool PreviewWindow::loadImageFile(const QString& path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize announcedSize = reader.size();
    if (announcedSize.isValid()
        && static_cast<qint64>(announcedSize.width()) * announcedSize.height()
            > kMaximumPreviewPixels) {
        QMessageBox::warning(
            this, QStringLiteral("影像尺寸超过预览限制"),
            QStringLiteral("该工具单幅预览上限为 4000 万像素。"
                           "当前影像为 %1 × %2 像素。")
                .arg(announcedSize.width())
                .arg(announcedSize.height()));
        return false;
    }
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::critical(
            this, QStringLiteral("无法打开影像"),
            QStringLiteral("读取失败：%1").arg(reader.errorString()));
        return false;
    }
    return setSourceImage(image, QFileInfo(path).fileName(), path);
}

void PreviewWindow::openImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开卫星影像快视图"), QString(),
        QStringLiteral("影像文件 (*.png *.jpg *.jpeg);;所有文件 (*)"));
    if (!path.isEmpty()) {
        loadImageFile(path);
    }
}

void PreviewWindow::savePreview()
{
    if (m_previewImage.isNull()) {
        QMessageBox::warning(this, QStringLiteral("没有可导出的预览"),
                             QStringLiteral("请先载入一幅影像。"));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出当前拉伸预览"),
        QStringLiteral("satellite-preview.png"),
        QStringLiteral("PNG 图像 (*.png)"));
    if (path.isEmpty()) {
        return;
    }
    if (!m_previewImage.save(path, "PNG")) {
        QMessageBox::critical(
            this, QStringLiteral("导出失败"),
            QStringLiteral("无法保存 PNG 文件：%1").arg(path));
        return;
    }

    m_statusInfo->setText(
        QStringLiteral("已导出预览图：%1").arg(QFileInfo(path).fileName()));
}

void PreviewWindow::refreshPreview()
{
    if (m_sourceImage.isNull()) {
        return;
    }

    m_result = ImageProcessor::process(m_sourceImage, currentSettings());
    if (m_result.image.isNull()) {
        QMessageBox::critical(
            this, QStringLiteral("预览处理失败"),
            QStringLiteral("无法生成当前波段组合的预览。"));
        return;
    }

    m_previewImage = m_result.image;
    m_canvas->setImage(m_previewImage);
    m_histogram->setHistograms(m_result.histograms);
}

void PreviewWindow::showDemoImage()
{
    const QImage demo = createSyntheticSatelliteChip();
    setSourceImage(demo,
                   QStringLiteral("合成演示图（非真实卫星影像）"),
                   QStringLiteral("程序生成的示意影像"));
}

void PreviewWindow::updatePixelReadout(const QPoint& pixel)
{
    if (pixel.x() < 0 || pixel.y() < 0
        || pixel.x() >= m_previewImage.width()
        || pixel.y() >= m_previewImage.height()) {
        m_pixelInfo->setText(QStringLiteral("像素：—"));
        return;
    }

    const QRgb value = m_previewImage.pixel(pixel);
    m_pixelInfo->setText(
        QStringLiteral("像素 (%1, %2)  RGB(%3, %4, %5)")
            .arg(pixel.x())
            .arg(pixel.y())
            .arg(qRed(value))
            .arg(qGreen(value))
            .arg(qBlue(value)));
}

bool PreviewWindow::setSourceImage(const QImage& image,
                                   const QString& description,
                                   const QString& origin)
{
    if (image.isNull()) {
        QMessageBox::critical(
            this, QStringLiteral("无效影像"),
            QStringLiteral("输入影像为空，未更改当前预览。"));
        return false;
    }

    const qint64 pixelCount =
        static_cast<qint64>(image.width()) * image.height();
    if (pixelCount > kMaximumPreviewPixels) {
        QMessageBox::warning(
            this, QStringLiteral("影像尺寸超过预览限制"),
            QStringLiteral("该工具单幅预览上限为 4000 万像素。"
                           "当前影像为 %1 × %2 像素。")
                .arg(image.width())
                .arg(image.height()));
        return false;
    }

    m_sourceImage = image.convertToFormat(QImage::Format_RGB32);
    const QSignalBlocker blockRed(m_redChannel);
    const QSignalBlocker blockGreen(m_greenChannel);
    const QSignalBlocker blockBlue(m_blueChannel);
    const QSignalBlocker blockLow(m_lowClip);
    const QSignalBlocker blockHigh(m_highClip);
    m_redChannel->setCurrentIndex(0);
    m_greenChannel->setCurrentIndex(1);
    m_blueChannel->setCurrentIndex(2);
    m_lowClip->setValue(2);
    m_highClip->setValue(2);
    setWindowTitle(QStringLiteral("%1 — 高分影像波段组合与拉伸预览")
                       .arg(description));
    m_imageInfo->setText(
        QStringLiteral("%1\n尺寸：%2 × %3 像素\n显示：8 位 RGB 快视图\n来源：%4")
            .arg(description)
            .arg(m_sourceImage.width())
            .arg(m_sourceImage.height())
            .arg(origin));
    m_statusInfo->setText(QStringLiteral("影像已载入：%1").arg(description));
    refreshPreview();
    m_canvas->fitImage();
    return true;
}

PreviewSettings PreviewWindow::currentSettings() const
{
    PreviewSettings settings;
    settings.sourceChannelForOutput = {
        m_redChannel->currentData().toInt(),
        m_greenChannel->currentData().toInt(),
        m_blueChannel->currentData().toInt()
    };
    settings.lowClipPercent = m_lowClip->value();
    settings.highClipPercent = m_highClip->value();
    return settings;
}
