#include "ImageReader.h"
#include "Barcode.h"
#include <QImageReader>
#include <QSaveFile>
#include <ZXing/ReadBarcode.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/Version.h>
#include <exception>

ScanResult readBarcode(const QImage& image)
{
    if (image.isNull())
        return {"", "The image could not be loaded."};
    QImage gray = image.convertToFormat(QImage::Format_Grayscale8);
    ZXing::ImageView view(gray.constBits(), gray.width(), gray.height(),
                         ZXing::ImageFormat::Lum, gray.bytesPerLine());
    ZXing::ReaderOptions options;
#if ZXING_VERSION_MAJOR >= 3
    // In ZXing 3, Code39 also includes Code32. Read our exact standard format.
    options.setFormats(ZXing::BarcodeFormat::Code39Std);
#else
    options.setFormats(ZXing::BarcodeFormat::Code39);
#endif
    options.setTryHarder(true);
    options.setTryRotate(true);
    options.setTryInvert(true);
    try {
        auto result = ZXing::ReadBarcode(view, options);
        if (!result.isValid()) {
            gray.invertPixels();
            ZXing::ImageView inverted(gray.constBits(), gray.width(), gray.height(),
                                      ZXing::ImageFormat::Lum, gray.bytesPerLine());
            result = ZXing::ReadBarcode(inverted, options);
        }
        if (!result.isValid())
            return {"", "No readable Code 39 barcode found. Keep all bars in view."};
        QString number = QString::fromStdString(result.text());
        if (!validateNumber(number).isEmpty())
            return {"", "The barcode must contain 6 digits starting with 1 to 9."};
        return {number, ""};
    } catch (const std::exception&) {
        return {"", "The barcode image could not be processed."};
    }
}

QImage loadImage(const QString& path, QString& error)
{
    error.clear();
    QImageReader::setAllocationLimit(64);
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull())
        error = "Cannot open this image: " + reader.errorString();
    return image;
}

bool saveBarcode(const QImage& image, const QString& path, QString& error)
{
    error.clear();
    if (image.isNull()) {
        error = "Generate a barcode before saving.";
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "PNG") || !file.commit()) {
        error = "Could not save the PNG. Choose a writable folder.";
        return false;
    }
    return true;
}
