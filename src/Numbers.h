#pragma once
#include <QString>

bool parseFixed(const QString& text, int places, qint64& result);
QString fixedText(qint64 value, int places);
qint64 lineAmount(qint64 unitPaisa, qint64 quantity);
bool validQuantity(const QString& unit, qint64 quantity);
