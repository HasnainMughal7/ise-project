#pragma once

#include <QImage>
#include <QString>

QString validateNumber(const QString& number);
QString encodeCode39(const QString& number);
QImage makeBarcode(const QString& number);
