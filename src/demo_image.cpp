#include "demo_image.h"

#include <QPainter>
#include <QPainterPath>

#include <array>
#include <cstdint>

namespace
{
uint32_t noise(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}
}

QImage createSyntheticSatelliteChip()
{
    constexpr int width = 1280;
    constexpr int height = 900;
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(QColor(QStringLiteral("#8b9a65")));

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient background(0, 0, width, height);
    background.setColorAt(0.0, QColor(QStringLiteral("#829566")));
    background.setColorAt(1.0, QColor(QStringLiteral("#b5a77a")));
    painter.fillRect(QRect(0, 0, width, height), QBrush(background));

    const std::array<QColor, 7> fieldColors{
        QColor("#9caa68"), QColor("#708b4f"), QColor("#c0a76b"),
        QColor("#849761"), QColor("#a2b36d"), QColor("#786d4e"),
        QColor("#93a873")
    };
    for (int index = 0; index < 18; ++index) {
        const int x = (index * 173 + 52) % width;
        const int y = (index * 127 + 25) % height;
        const int fieldWidth = 150 + (index * 47) % 240;
        const int fieldHeight = 90 + (index * 31) % 185;
        QPainterPath field;
        field.moveTo(x, y);
        field.lineTo(x + fieldWidth, y - 24);
        field.lineTo(x + fieldWidth - 18, y + fieldHeight);
        field.lineTo(x - 35, y + fieldHeight - 17);
        field.closeSubpath();

        painter.fillPath(field, fieldColors[index % fieldColors.size()]);
        painter.setPen(QPen(QColor(226, 218, 183, 120), 2));
        painter.drawPath(field);

        painter.save();
        painter.setClipPath(field);
        painter.setPen(QPen(QColor(235, 231, 195, 34), 1));
        for (int line = -height; line < width; line += 11) {
            painter.drawLine(line, 0, line + height, height);
        }
        painter.restore();
    }

    QPainterPath river;
    river.moveTo(-40, 560);
    river.cubicTo(220, 490, 310, 675, 555, 617);
    river.cubicTo(810, 553, 910, 718, 1320, 584);
    painter.setPen(QPen(QColor("#668f9a"), 47,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(river);
    painter.setPen(QPen(QColor("#91afb0"), 19,
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(river);

    painter.setPen(QPen(QColor("#c8b995"), 14, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(QPolygon({QPoint(-30, 162), QPoint(240, 219),
                                   QPoint(565, 186), QPoint(830, 271),
                                   QPoint(1320, 230)}));
    painter.setPen(QPen(QColor("#e0d3b3"), 5));
    painter.drawPolyline(QPolygon({QPoint(-30, 162), QPoint(240, 219),
                                   QPoint(565, 186), QPoint(830, 271),
                                   QPoint(1320, 230)}));

    painter.setPen(Qt::NoPen);
    for (int index = 0; index < 85; ++index) {
        const int x = 360 + (index * 29) % 310;
        const int y = 300 + (index * 43) % 200;
        const int size = 7 + (index * 13) % 18;
        painter.setBrush((index % 4) == 0
                             ? QColor("#d0c5a4")
                             : QColor("#b8ad91"));
        painter.drawRect(x, y, size, size / 2 + 4);
    }

    painter.end();

    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const int delta = static_cast<int>(
                noise(static_cast<uint32_t>(x * 73856093U)
                      ^ static_cast<uint32_t>(y * 19349663U)) % 13U) - 6;
            row[x] = qRgb(qBound(0, qRed(row[x]) + delta, 255),
                          qBound(0, qGreen(row[x]) + delta, 255),
                          qBound(0, qBlue(row[x]) + delta, 255));
        }
    }
    return image;
}
