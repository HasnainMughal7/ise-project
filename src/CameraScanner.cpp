#include "CameraScanner.h"
#include "ImageReader.h"
#include <QMediaDevices>
#include <QCameraDevice>
#include <QVideoFrame>

CameraScanner::CameraScanner(QObject* parent) : QObject(parent)
{
    session.setCamera(&camera);
    session.setVideoSink(&sink);
    watchdog.setSingleShot(true);
    connect(&sink, &QVideoSink::videoFrameChanged, this,
            &CameraScanner::processFrame, Qt::QueuedConnection);
    connect(&camera, &QCamera::errorOccurred, this,
            [this](QCamera::Error, const QString& message) {
        if (!running) return;
        stop();
        emit problem("Camera error: " + message);
    });
    connect(&watchdog, &QTimer::timeout, this, [this]() {
        stop();
        emit problem("No camera frames. Check camera permissions and close other camera apps.");
    });
}

CameraScanner::~CameraScanner()
{
    stop();
}

void CameraScanner::start(int deviceIndex)
{
    stop();
    auto devices = QMediaDevices::videoInputs();
    if (deviceIndex < 0 || deviceIndex >= devices.size()) {
        emit problem("No camera found. Connect a webcam and restart the app.");
        return;
    }
    camera.setCameraDevice(devices[deviceIndex]);
    candidate.clear();
    matches = 0;
    running = true;
    clock.start();
    watchdog.start(8000);
    emit runningChanged(true);
    camera.start();
}

void CameraScanner::stop()
{
    running = false;
    watchdog.stop();
    camera.stop();
    emit runningChanged(false);
}

void CameraScanner::processFrame(const QVideoFrame& frame)
{
    if (!running || !frame.isValid()) return;
    if (clock.elapsed() < 150) return;
    clock.restart();
    QImage image = frame.toImage();
    if (image.isNull()) return;
    watchdog.start(8000);
    image = image.scaled(1280, 960, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    emit frameReady(image);
    ScanResult result = readBarcode(image);
    if (!result.ok()) {
        candidate.clear();
        matches = 0;
        return;
    }
    if (result.number == candidate) {
        ++matches;
    } else {
        candidate = result.number;
        matches = 1;
    }
    if (matches >= 2) {
        stop();
        emit codeFound(result.number);
    }
}
