#pragma once
#include "Models.h"
QString receiptHtml(const Order& order);
bool exportPdf(const Order& order, const QString& path, QString& error);
