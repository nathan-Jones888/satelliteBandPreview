#pragma once

#include "image_processor.h"

#include <QMainWindow>
#include <QImage>

class HistogramWidget;
class ImageCanvas;
class QLabel;
class QComboBox;
class QSpinBox;

class PreviewWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreviewWindow(QWidget* parent = nullptr);
    bool loadImageFile(const QString& path);

private slots:
    void openImage();
    void savePreview();
    void refreshPreview();
    void showDemoImage();
    void updatePixelReadout(const QPoint& pixel);

private:
    bool setSourceImage(const QImage& image,
                        const QString& description,
                        const QString& origin);
    PreviewSettings currentSettings() const;

    QImage m_sourceImage;
    QImage m_previewImage;
    PreviewResult m_result;
    ImageCanvas* m_canvas = nullptr;
    HistogramWidget* m_histogram = nullptr;
    QComboBox* m_redChannel = nullptr;
    QComboBox* m_greenChannel = nullptr;
    QComboBox* m_blueChannel = nullptr;
    QSpinBox* m_lowClip = nullptr;
    QSpinBox* m_highClip = nullptr;
    QLabel* m_imageInfo = nullptr;
    QLabel* m_pixelInfo = nullptr;
    QLabel* m_statusInfo = nullptr;
};
