#include "MainWindow.h"
#include "Barcode.h"
#include "ImageReader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QMediaDevices>
#include <QCameraDevice>

MainWindow::MainWindow()
{
    setWindowTitle("Barcode Studio - Increment 1");
    setMinimumSize(800, 650);
    resize(920, 700);
    setStyleSheet(
        "QWidget { background: #101827; color: #ecf2fb; font-size: 15px; }"
        "QLabel#title { font-size: 30px; font-weight: bold; }"
        "QLabel#hint { color: #acbcd0; }"
        "QLabel#preview { background: white; color: #526074; border-radius: 12px; }"
        "QLabel#result { font-size: 32px; font-weight: bold; color: #69e0c0; }"
        "QLineEdit, QComboBox { background: #1c2940; border: 1px solid #53627b;"
        " border-radius: 7px; padding: 10px; }"
        "QLineEdit:focus { border: 2px solid #69e0c0; }"
        "QPushButton { background: #3062d8; border: none; border-radius: 7px;"
        " padding: 12px 18px; font-weight: bold; }"
        "QPushButton:hover { background: #4277ee; }"
        "QPushButton:disabled { background: #283449; color: #8290a5; }"
        "QPushButton#secondary { background: #293950; }"
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { background: #1c2940; padding: 12px 28px; margin: 4px; }"
        "QTabBar::tab:selected { background: #3062d8; }"
    );
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(14);
    auto title = new QLabel("Barcode Studio");
    title->setObjectName("title");
    auto hint = new QLabel("Six digits. One barcode. Ready to read.");
    hint->setObjectName("hint");
    layout->addWidget(title);
    layout->addWidget(hint);
    tabs = new QTabWidget;
    tabs->addTab(makeGeneratePage(), "Generate");
    tabs->addTab(makeReadPage(), "Read barcode");
    layout->addWidget(tabs);
    connect(tabs, &QTabWidget::currentChanged, this, [this](int) {
        scanner.stop();
        scanStatus->setText("Upload a PNG or JPG, or start the camera.");
    });
    connect(&scanner, &CameraScanner::frameReady, this, [this](const QImage& image) {
        showImage(scanPreview, image);
    });
    connect(&scanner, &CameraScanner::codeFound, this, [this](const QString& number) {
        resultLabel->setText(number);
        scanStatus->setText("Barcode read successfully. Camera stopped.");
    });
    connect(&scanner, &CameraScanner::problem, scanStatus, &QLabel::setText);
    connect(&scanner, &CameraScanner::runningChanged, this, [this](bool running) {
        startButton->setEnabled(!running);
        stopButton->setEnabled(running);
        cameras->setEnabled(!running);
    });
}

QWidget* MainWindow::makeGeneratePage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->setSpacing(14);
    layout->addWidget(new QLabel("Product number   /   6 digits, first digit 1 to 9"));
    input = new QLineEdit;
    input->setObjectName("numberInput");
    input->setPlaceholderText("Example: 123456");
    input->setMaxLength(32);
    layout->addWidget(input);
    auto buttons = new QHBoxLayout;
    auto generateButton = new QPushButton("Generate + save PNG");
    generateButton->setObjectName("generateButton");
    saveButton = new QPushButton("Save as...");
    saveButton->setObjectName("secondary");
    saveButton->setEnabled(false);
    buttons->addWidget(generateButton);
    buttons->addWidget(saveButton);
    layout->addLayout(buttons);
    generatedPreview = new QLabel("Your barcode will appear here");
    generatedPreview->setObjectName("preview");
    generatedPreview->setAlignment(Qt::AlignCenter);
    generatedPreview->setMinimumHeight(270);
    layout->addWidget(generatedPreview, 1);
    generateStatus = new QLabel("Code 39 encoding written in C++. PNGs save to Pictures/BarcodeStudio.");
    generateStatus->setWordWrap(true);
    generateStatus->setMinimumHeight(62);
    generateStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(generateStatus);
    connect(generateButton, &QPushButton::clicked, this, &MainWindow::generate);
    connect(input, &QLineEdit::returnPressed, this, &MainWindow::generate);
    connect(input, &QLineEdit::textChanged, this, &MainWindow::clearGenerated);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveAs);
    return page;
}

QWidget* MainWindow::makeReadPage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->setSpacing(12);
    auto buttons = new QHBoxLayout;
    auto uploadButton = new QPushButton("Upload image");
    startButton = new QPushButton("Start camera");
    stopButton = new QPushButton("Stop");
    stopButton->setObjectName("secondary");
    stopButton->setEnabled(false);
    auto clearButton = new QPushButton("Clear");
    clearButton->setObjectName("secondary");
    buttons->addWidget(uploadButton);
    buttons->addWidget(startButton);
    buttons->addWidget(stopButton);
    buttons->addWidget(clearButton);
    layout->addLayout(buttons);
    cameras = new QComboBox;
    for (const QCameraDevice& device : QMediaDevices::videoInputs())
        cameras->addItem(device.description());
    cameras->setPlaceholderText("No camera connected");
    layout->addWidget(cameras);
    scanPreview = new QLabel("Upload a barcode image or hold one in front of the camera");
    scanPreview->setObjectName("preview");
    scanPreview->setAlignment(Qt::AlignCenter);
    scanPreview->setMinimumHeight(235);
    layout->addWidget(scanPreview, 1);
    resultLabel = new QLabel("------");
    resultLabel->setObjectName("result");
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(resultLabel);
    scanStatus = new QLabel("Keep the complete barcode and its white margins visible.");
    scanStatus->setWordWrap(true);
    scanStatus->setMinimumHeight(44);
    layout->addWidget(scanStatus);
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::upload);
    connect(startButton, &QPushButton::clicked, this, [this]() {
        clearScan();
        scanStatus->setText("Scanning... Keep the barcode steady and well lit.");
        scanner.start(cameras->currentIndex());
    });
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        scanner.stop();
        scanStatus->setText("Camera stopped.");
    });
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearScan);
    return page;
}

void MainWindow::generate()
{
    clearGenerated();
    QString error = validateNumber(input->text());
    if (!error.isEmpty()) {
        generateStatus->setText(error);
        return;
    }
    generatedNumber = input->text();
    generated = makeBarcode(generatedNumber);
    showImage(generatedPreview, generated);
    saveButton->setEnabled(true);
    QString folder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (folder.isEmpty()) folder = QDir::homePath();
    folder += "/BarcodeStudio";
    QString path = folder + "/barcode_" + generatedNumber + ".png";
    if (!QDir().mkpath(folder) || !saveBarcode(generated, path, error)) {
        generateStatus->setText("Barcode generated, but automatic saving failed. Use Save as...");
        return;
    }
    generateStatus->setText("Generated " + generatedNumber + " and saved PNG to:\n" + path);
}

void MainWindow::saveAs()
{
    if (generated.isNull()) return;
    QString path = QFileDialog::getSaveFileName(this, "Save barcode",
        "barcode_" + generatedNumber + ".png", "PNG image (*.png)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
    QString error;
    if (saveBarcode(generated, path, error))
        generateStatus->setText("PNG saved to:\n" + path);
    else
        generateStatus->setText(error);
}

void MainWindow::upload()
{
    QString path = QFileDialog::getOpenFileName(this, "Open barcode image", "",
        "Images (*.png *.jpg *.jpeg *.bmp);;All files (*)");
    if (path.isEmpty()) return;
    clearScan();
    QString error;
    QImage image = loadImage(path, error);
    if (image.isNull()) {
        scanStatus->setText(error);
        return;
    }
    showImage(scanPreview, image);
    ScanResult result = readBarcode(image);
    if (result.ok()) {
        resultLabel->setText(result.number);
        scanStatus->setText("Barcode read successfully.");
    } else {
        scanStatus->setText(result.error);
    }
}

void MainWindow::clearGenerated()
{
    generated = QImage();
    generatedNumber.clear();
    generatedPreview->setText("Your barcode will appear here");
    saveButton->setEnabled(false);
    generateStatus->setText("Enter 6 digits, then generate your barcode.");
}

void MainWindow::clearScan()
{
    scanner.stop();
    resultLabel->setText("------");
    scanPreview->setText("Upload a barcode image or start the camera");
    scanStatus->setText("Ready to scan.");
}

void MainWindow::showImage(QLabel* label, const QImage& image)
{
    QSize size = label->size() - QSize(24, 24);
    label->setPixmap(QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio,
                                                   Qt::FastTransformation)));
}
