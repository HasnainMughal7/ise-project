#pragma once

#include "CameraScanner.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTabWidget>

class MainWindow : public QWidget {
public:
    MainWindow();
private:
    QWidget* makeGeneratePage();
    QWidget* makeReadPage();
    void generate();
    void saveAs();
    void upload();
    void clearGenerated();
    void clearScan();
    void showImage(QLabel* label, const QImage& image);
    CameraScanner scanner;
    QTabWidget* tabs;
    QLineEdit* input;
    QLabel* generatedPreview;
    QLabel* generateStatus;
    QPushButton* saveButton;
    QLabel* scanPreview;
    QLabel* scanStatus;
    QLabel* resultLabel;
    QComboBox* cameras;
    QPushButton* startButton;
    QPushButton* stopButton;
    QImage generated;
    QString generatedNumber;
};
