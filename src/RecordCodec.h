#pragma once
#include <QStringList>

bool splitRecord(const QString& line, QStringList& fields);
QString joinRecord(const QStringList& fields);
