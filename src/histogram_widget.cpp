#include "histogram_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include <algorithm>

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void HistogramWidget::setHistograms(
    const std::array<std::array<quint64, 256>, 3>& histograms)
{
    m_histograms = histograms;
    m_hasData = true;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF plot(34.0, 16.0, width() - 48.0, height() - 48.0);
    painter.setPen(QColor(QStringLiteral("#667085")));
    painter.drawText(QRect(10, height() - 26, width() - 20, 18),
                     Qt::AlignCenter,
                     QStringLiteral("像素亮度值 0–255"));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    if (!m_hasData || plot.width() <= 0.0 || plot.height() <= 0.0) {
        painter.drawText(plot, Qt::AlignCenter,
                         QStringLiteral("载入影像后显示直方图"));
        return;
    }

    quint64 maximum = 1;
    for (const auto& histogram : m_histograms) {
        for (quint64 count : histogram) {
            maximum = std::max(maximum, count);
        }
    }

    const std::array<QColor, 3> colors{
        QColor(217, 45, 32, 175),
        QColor(3, 152, 85, 175),
        QColor(21, 112, 239, 175)
    };
    for (int channel = 0; channel < 3; ++channel) {
        QPainterPath path;
        for (int value = 0; value < 256; ++value) {
            const double x = plot.left()
                + (static_cast<double>(value) / 255.0) * plot.width();
            const double ratio =
                static_cast<double>(m_histograms[channel][value])
                / static_cast<double>(maximum);
            const double y = plot.bottom() - ratio * plot.height();
            if (value == 0) {
                path.moveTo(x, plot.bottom());
                path.lineTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        path.lineTo(plot.right(), plot.bottom());
        painter.setPen(QPen(colors[channel], 1.0));
        painter.setBrush(QBrush(colors[channel]));
        painter.drawPath(path);
    }
}
