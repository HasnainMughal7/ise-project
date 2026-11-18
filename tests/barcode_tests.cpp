#include "Barcode.h"
#include "ImageReader.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QTransform>
#include <QPainter>
#include <QFile>
#include <iostream>

int failures = 0;
int checks = 0;

void check(bool passed, const QString& name)
{
    ++checks;
    if (!passed) {
        ++failures;
        std::cerr << "FAIL: " << name.toStdString() << '\n';
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QStringList invalid = {"", "12345", "1234567", "012345", "000000",
                           "12a456", "12 456", "+12345", "123456 ", " 123456"};
    for (const QString& number : invalid) {
        check(!validateNumber(number).isEmpty(), "reject " + number);
        check(makeBarcode(number).isNull(), "no image for " + number);
    }
    QStringList valid = {"100000", "999999", "123456", "345678", "888888", "987654"};
    for (int i = 100000; i <= 999999; i += 7919)
        valid.append(QString::number(i));
    QTemporaryDir temp;
    check(temp.isValid(), "temporary directory");
    for (const QString& number : valid) {
        QImage image = makeBarcode(number);
        check(encodeCode39(number).size() == 147, "module count " + number);
        check(image.size() == QSize(588, 240), "image size " + number);
        check(readBarcode(image).number == number, "round trip " + number);
        QString error;
        QString path = temp.path() + "/barcode_" + number + ".png";
        check(saveBarcode(image, path, error), "save " + number);
        check(readBarcode(loadImage(path, error)).number == number, "reload " + number);
    }
    QImage image = makeBarcode("345678");
    for (int angle : {90, 180, 270}) {
        QImage rotated = image.transformed(QTransform().rotate(angle));
        check(readBarcode(rotated).number == "345678", "rotation " + QString::number(angle));
    }
    image.invertPixels();
    check(readBarcode(image).number == "345678", "inverted colours");
    image.invertPixels();
    check(readBarcode(image.scaledToWidth(400)).number == "345678", "smaller image");
    QPainter painter(&image);
    painter.fillRect(0, 192, image.width(), 48, Qt::white);
    painter.end();
    check(readBarcode(image).number == "345678", "reads bars without printed digits");
    QImage blank(600, 240, QImage::Format_RGB32);
    blank.fill(Qt::white);
    check(!readBarcode(blank).ok(), "blank image rejected");
    check(!readBarcode(QImage()).ok(), "empty image rejected");
    QString error;
    check(loadImage(temp.path() + "/missing.png", error).isNull(), "missing file");
    check(!error.isEmpty(), "missing file explanation");
    QFile broken(temp.path() + "/broken.png");
    if (!broken.open(QIODevice::WriteOnly)) {
        std::cerr << "Cannot create the corrupt-image test file.\n";
        return 1;
    }
    broken.write("This is not an image");
    broken.close();
    check(loadImage(broken.fileName(), error).isNull(), "corrupt image");
    check(!saveBarcode(blank, temp.path() + "/missing/out.png", error), "save failure");
    check(!saveBarcode(QImage(), temp.path() + "/empty.png", error), "empty save");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
