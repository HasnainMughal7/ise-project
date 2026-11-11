#pragma once

#include <QCamera>
#include <QElapsedTimer>
#include <QImage>
#include <QMediaCaptureSession>
#include <QTimer>
#include <QVideoSink>

class CameraScanner : public QObject {
    Q_OBJECT
public:
    explicit CameraScanner(QObject* parent = nullptr);
    ~CameraScanner();
    void start(int deviceIndex);
    void stop();
    void processFrame(const QVideoFrame& frame);
signals:
    void frameReady(const QImage& image);
    void codeFound(const QString& number);
    void problem(const QString& message);
    void runningChanged(bool running);
private:
    QCamera camera;
    QMediaCaptureSession session;
    QVideoSink sink;
    QElapsedTimer clock;
    QTimer watchdog;
    bool running = false;
    QString candidate;
    int matches = 0;
};
