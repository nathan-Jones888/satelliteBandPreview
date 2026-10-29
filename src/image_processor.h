#pragma once

#include <QImage>

#include <array>

struct PreviewSettings
{
    std::array<int, 3> sourceChannelForOutput{{0, 1, 2}};
    int lowClipPercent = 2;
    int highClipPercent = 2;
};

struct PreviewResult
{
    QImage image;
    std::array<std::array<quint64, 256>, 3> histograms{};
    std::array<int, 3> lowValues{};
    std::array<int, 3> highValues{};
};

class ImageProcessor final
{
public:
    static PreviewResult process(const QImage& source,
                                 const PreviewSettings& settings);

private:
    static int channelValue(QRgb pixel, int channel);
    static int percentileValue(const std::array<quint64, 256>& histogram,
                               quint64 pixelCount,
                               int percentile);
    static int stretchValue(int value, int low, int high);
};
