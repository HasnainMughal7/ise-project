#pragma once

#include <QImage>
#include <QString>

struct ScanResult {
    QString number;
    QString error;
    bool ok() const { return !number.isEmpty(); }
};

ScanResult readBarcode(const QImage& image);
QImage loadImage(const QString& path, QString& error);
bool saveBarcode(const QImage& image, const QString& path, QString& error);
