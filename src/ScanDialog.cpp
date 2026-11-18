#include "ScanDialog.h"
#include <QCameraDevice>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaDevices>
#include <QPushButton>
#include <QVBoxLayout>

ScanDialog::ScanDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Scan a product barcode");
    resize(720, 580);
    auto layout = new QVBoxLayout(this);
    auto title = new QLabel("Keep the complete barcode and white margins in view.");
    title->setWordWrap(true);
    layout->addWidget(title);
    auto devices = new QComboBox;
    for (const QCameraDevice& device : QMediaDevices::videoInputs()) devices->addItem(device.description());
    layout->addWidget(devices);
    auto preview = new QLabel("Camera preview");
    preview->setMinimumSize(640, 360);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet("background:#172b3b;color:white;border-radius:10px;");
    layout->addWidget(preview, 1);
    auto status = new QLabel("Choose the laptop camera, then press Start.");
    status->setWordWrap(true);
    status->setTextFormat(Qt::PlainText);
    layout->addWidget(status);
    auto buttons = new QHBoxLayout;
    auto start = new QPushButton("Start camera");
    auto stop = new QPushButton("Stop");
    auto cancel = new QPushButton("Cancel");
    buttons->addWidget(start); buttons->addWidget(stop); buttons->addWidget(cancel);
    layout->addLayout(buttons);
    stop->setEnabled(false);
    start->setEnabled(devices->count() > 0);
    if (devices->count() == 0) status->setText("No camera found. Connect it, then reopen this window.");
    connect(start, &QPushButton::clicked, this, [this, devices, status]() {
        status->setText("Scanning... use clear light and avoid glare.");
        scanner.start(devices->currentIndex());
    });
    connect(stop, &QPushButton::clicked, this, [this, status]() { scanner.stop(); status->setText("Camera stopped."); });
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(&scanner, &CameraScanner::frameReady, this, [preview](const QImage& image) {
        preview->setPixmap(QPixmap::fromImage(image).scaled(preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    connect(&scanner, &CameraScanner::problem, status, &QLabel::setText);
    connect(&scanner, &CameraScanner::runningChanged, this, [start, stop, devices](bool running) {
        start->setEnabled(!running && devices->count() > 0);
        stop->setEnabled(running);
        devices->setEnabled(!running);
    });
    connect(&scanner, &CameraScanner::codeFound, this, [this](const QString& code) { result = code; accept(); });
    connect(this, &QDialog::finished, this, [this]() { scanner.stop(); });
}
