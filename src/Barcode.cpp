#include "Barcode.h"
#include <QPainter>

QString validateNumber(const QString& number)
{
    if (number.size() != 6)
        return "Enter exactly 6 digits.";
    for (QChar digit : number) {
        if (digit < QChar('0') || digit > QChar('9'))
            return "Use digits 0 to 9 only.";
    }
    if (number[0] == QChar('0'))
        return "The first digit must be 1 to 9.";
    return "";
}

QString encodeCode39(const QString& number)
{
    if (!validateNumber(number).isEmpty())
        return "";
    // Each row describes 9 alternating bars and spaces. 1 = narrow, 3 = wide.
    const QString patterns[10] = {
        "111331311", "311311113", "113311113", "313311111", "111331113",
        "311331111", "113331111", "111311313", "311311311", "113311311"
    };
    const QString star = "131131311";
    QString symbols = "*" + number + "*";
    QString modules(10, '0');
    for (int i = 0; i < symbols.size(); ++i) {
        QString pattern = star;
        if (symbols[i] != QChar('*'))
            pattern = patterns[symbols[i].digitValue()];
        for (int j = 0; j < 9; ++j) {
            int width = pattern[j].digitValue();
            QChar colour = (j % 2 == 0) ? QChar('1') : QChar('0');
            modules += QString(width, colour);
        }
        if (i < symbols.size() - 1)
            modules += '0';
    }
    modules += QString(10, '0');
    return modules;
}

QImage makeBarcode(const QString& number)
{
    QString modules = encodeCode39(number);
    if (modules.isEmpty())
        return QImage();
    const int scale = 4;
    QImage image(modules.size() * scale, 240, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    for (int x = 0; x < modules.size(); ++x) {
        if (modules[x] == QChar('1'))
            painter.fillRect(x * scale, 20, scale, 170, Qt::black);
    }
    QFont font = painter.font();
    font.setPixelSize(26);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(0, 194, image.width(), 40, Qt::AlignCenter, number);
    return image;
}
