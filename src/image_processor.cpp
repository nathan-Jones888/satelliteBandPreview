#include "image_processor.h"

#include <algorithm>
#include <cmath>

PreviewResult ImageProcessor::process(const QImage& source,
                                      const PreviewSettings& settings)
{
    PreviewResult result;
    if (source.isNull()) {
        return result;
    }

    const QImage rgbSource = source.convertToFormat(QImage::Format_RGB32);
    const quint64 pixelCount =
        static_cast<quint64>(rgbSource.width()) * rgbSource.height();
    const auto channelIndex = [](int channel) {
        return std::clamp(channel, 0, 2);
    };

    for (int y = 0; y < rgbSource.height(); ++y) {
        const auto* row = reinterpret_cast<const QRgb*>(
            rgbSource.constScanLine(y));
        for (int x = 0; x < rgbSource.width(); ++x) {
            for (int outputChannel = 0; outputChannel < 3; ++outputChannel) {
                const int inputChannel =
                    channelIndex(settings.sourceChannelForOutput[outputChannel]);
                const int value = channelValue(row[x], inputChannel);
                ++result.histograms[outputChannel][value];
            }
        }
    }

    const int lowClip = std::clamp(settings.lowClipPercent, 0, 49);
    const int highClip = std::clamp(settings.highClipPercent, 0, 49);
    std::array<std::array<quint8, 256>, 3> lookupTables{};
    for (int outputChannel = 0; outputChannel < 3; ++outputChannel) {
        const auto& histogram = result.histograms[outputChannel];
        const int low = percentileValue(histogram, pixelCount, lowClip);
        const int high = percentileValue(histogram,
                                         pixelCount,
                                         100 - highClip);
        result.lowValues[outputChannel] = low;
        result.highValues[outputChannel] = high;
        for (int value = 0; value < 256; ++value) {
            lookupTables[outputChannel][value] =
                static_cast<quint8>(stretchValue(value, low, high));
        }
    }

    result.image = QImage(rgbSource.size(), QImage::Format_RGB32);
    for (int y = 0; y < rgbSource.height(); ++y) {
        const auto* sourceRow = reinterpret_cast<const QRgb*>(
            rgbSource.constScanLine(y));
        auto* targetRow = reinterpret_cast<QRgb*>(
            result.image.scanLine(y));
        for (int x = 0; x < rgbSource.width(); ++x) {
            const int red = channelValue(
                sourceRow[x],
                channelIndex(settings.sourceChannelForOutput[0]));
            const int green = channelValue(
                sourceRow[x],
                channelIndex(settings.sourceChannelForOutput[1]));
            const int blue = channelValue(
                sourceRow[x],
                channelIndex(settings.sourceChannelForOutput[2]));
            targetRow[x] = qRgb(lookupTables[0][red],
                                lookupTables[1][green],
                                lookupTables[2][blue]);
        }
    }

    return result;
}

int ImageProcessor::channelValue(QRgb pixel, int channel)
{
    switch (channel) {
    case 0:
        return qRed(pixel);
    case 1:
        return qGreen(pixel);
    default:
        return qBlue(pixel);
    }
}

int ImageProcessor::percentileValue(
    const std::array<quint64, 256>& histogram,
    quint64 pixelCount,
    int percentile)
{
    if (pixelCount == 0) {
        return 0;
    }

    const int clampedPercentile = std::clamp(percentile, 0, 100);
    if (clampedPercentile == 100) {
        for (int value = 255; value >= 0; --value) {
            if (histogram[value] > 0) {
                return value;
            }
        }
        return 0;
    }

    const quint64 target = static_cast<quint64>(
        std::floor(static_cast<double>(pixelCount)
                   * clampedPercentile / 100.0));
    quint64 accumulated = 0;
    for (int value = 0; value < 256; ++value) {
        accumulated += histogram[value];
        if (accumulated > target) {
            return value;
        }
    }
    return 255;
}

int ImageProcessor::stretchValue(int value, int low, int high)
{
    if (high <= low) {
        return 128;
    }

    const double scaled =
        (static_cast<double>(value - low) * 255.0) / (high - low);
    return std::clamp(static_cast<int>(std::lround(scaled)), 0, 255);
}
