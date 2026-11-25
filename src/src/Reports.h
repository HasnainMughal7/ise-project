#pragma once
#include "Models.h"
#include <QStringList>
enum class ReportKind { Incomplete, Completed, Cash, Sales, BestSellers, HighestStock };
enum class Period { Day, Week, Month, Year, Custom };
struct Report {
    QStringList columns;
    QVector<QStringList> rows;
    QString summary;
};
QPair<QDate, QDate> periodRange(Period period, QDate day);
Report makeReport(const StoreState& state, ReportKind kind, QDate from, QDate through);
