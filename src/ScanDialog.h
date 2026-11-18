#pragma once
#include "CameraScanner.h"
#include <QDialog>

class ScanDialog : public QDialog {
public:
    explicit ScanDialog(QWidget* parent = nullptr);
    QString barcode() const { return result; }
private:
    CameraScanner scanner;
    QString result;
};
