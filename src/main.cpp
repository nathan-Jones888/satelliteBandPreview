#include "demo_image.h"
#include "image_processor.h"
#include "preview_window.h"

#include <QApplication>
#include <QBuffer>
#include <QDebug>
#include <QStringList>

namespace
{
int runSelfTest()
{
    QImage source(4, 1, QImage::Format_RGB32);
    source.setPixel(0, 0, qRgb(0, 10, 20));
    source.setPixel(1, 0, qRgb(64, 80, 96));
    source.setPixel(2, 0, qRgb(128, 144, 160));
    source.setPixel(3, 0, qRgb(255, 240, 220));

    PreviewSettings settings;
    settings.sourceChannelForOutput = {{2, 1, 0}};
    settings.lowClipPercent = 0;
    settings.highClipPercent = 0;
    const PreviewResult result = ImageProcessor::process(source, settings);

    if (result.image.size() != source.size()) {
        qCritical() << "Self-test failed: output size changed.";
        return 1;
    }
    if (result.image.pixel(0, 0) != qRgb(0, 0, 0)
        || result.image.pixel(3, 0) != qRgb(255, 255, 255)) {
        qCritical() << "Self-test failed: full-range stretch is incorrect.";
        return 1;
    }
    if (result.histograms[0][20] != 1
        || result.histograms[1][80] != 1
        || result.histograms[2][64] != 1) {
        qCritical() << "Self-test failed: selected-channel histograms are incorrect.";
        return 1;
    }

    QBuffer pngBuffer;
    if (!pngBuffer.open(QIODevice::ReadWrite)
        || !result.image.save(&pngBuffer, "PNG")) {
        qCritical() << "Self-test failed: PNG encoding is unavailable.";
        return 1;
    }
    QImage decodedPng;
    if (!decodedPng.loadFromData(pngBuffer.data(), "PNG")
        || decodedPng != result.image) {
        qCritical() << "Self-test failed: PNG round trip changed the preview.";
        return 1;
    }

    QImage constantImage(3, 1, QImage::Format_RGB32);
    constantImage.fill(qRgb(72, 72, 72));
    const PreviewResult constantResult =
        ImageProcessor::process(constantImage, settings);
    if (constantResult.image.pixel(1, 0) != qRgb(128, 128, 128)) {
        qCritical() << "Self-test failed: flat input did not map to neutral gray.";
        return 1;
    }

    const QImage demo = createSyntheticSatelliteChip();
    if (demo.isNull() || demo.size() != QSize(1280, 900)) {
        qCritical() << "Self-test failed: synthetic demonstration image is invalid.";
        return 1;
    }

    qInfo() << "Self-test passed: channel mapping, histogram, stretch, and demo image.";
    return 0;
}
}

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(
        QStringLiteral("SatelliteBandPreview"));
    QCoreApplication::setOrganizationName(
        QStringLiteral("Satellite Image Tools Demo"));

    const QStringList arguments = application.arguments();
    if (arguments.contains(QStringLiteral("--self-test"))) {
        return runSelfTest();
    }

    PreviewWindow window;
    if (arguments.size() > 1) {
        window.loadImageFile(arguments.at(1));
    }
    window.show();
    return application.exec();
}
