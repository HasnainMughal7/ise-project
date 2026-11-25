#include "Numbers.h"
#include <QRegularExpression>
#include <QStringList>

bool parseFixed(const QString& text, int places, qint64& result)
{
    if (places < 0 || places > 3) return false;
    QString rule = "\\A[0-9]{1,15}(\\.[0-9]{1," + QString::number(places) + "})?\\z";
    if (places == 0) rule = "\\A[0-9]{1,15}\\z";
    if (!QRegularExpression(rule).match(text).hasMatch()) return false;
    QStringList parts = text.split('.');
    qint64 scale = 1;
    for (int i = 0; i < places; ++i) scale *= 10;
    QString fraction = parts.size() == 2 ? parts[1] : "";
    while (fraction.size() < places) fraction += '0';
    result = parts[0].toLongLong() * scale + fraction.toLongLong();
    return true;
}

QString fixedText(qint64 value, int places)
{
    qint64 scale = 1;
    for (int i = 0; i < places; ++i) scale *= 10;
    if (places == 0) return QString::number(value);
    return QString::number(value / scale) + "." +
           QString::number(value % scale).rightJustified(places, '0');
}

qint64 lineAmount(qint64 unitPaisa, qint64 quantity)
{
    // Round each nonnegative line component to the nearest paisa, half up.
    return (unitPaisa * quantity + 500) / 1000;
}

bool validQuantity(const QString& unit, qint64 quantity)
{
    if (quantity <= 0 || quantity > 1000000000) return false;
    if (unit == "count") return quantity % 1000 == 0;
    return unit == "weight";
}
