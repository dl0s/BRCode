#include <bb/cascades/multimedia/Camera>
#include <bb/cascades/multimedia/BarcodeDetector>
#include <bb/cascades/multimedia/BarcodeGenerator>
#include <bb/cascades/multimedia/BarcodeGeneratorFormat>
#include <bb/cascades/multimedia/BarcodeFormat>
#include <bb/ImageData>
#include <QUrl>
using namespace bb::cascades::multimedia;
void brcodeSdkSpike(Camera *camera) {
    BarcodeDetector detector;
    detector.setCamera(camera);
    detector.setFormats(BarcodeFormat::QrCode);
    BarcodeGenerator generator;
    bb::ImageData data=generator.generate(BarcodeGeneratorFormat::QrCode, QString::fromUtf8("BRCode\n中文"), 1.0f);
    detector.scanImage(data);
    generator.generate(QUrl::fromLocalFile("/tmp/brcode.png"), BarcodeGeneratorFormat::QrCode, "BRCode", 4.0f);
    generator.error();
}
