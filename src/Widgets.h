#pragma once
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QLocale>
#include <QHBoxLayout>
#include <QDateEdit>
inline QPushButton* button(QString text, QString id = {}) {
    auto* b = new QPushButton(text);
    b->setObjectName(id);
    return b;
}
inline QTableWidget* table(QStringList names) {
    auto* t = new QTableWidget;
    t->setColumnCount(names.size());
    t->setHorizontalHeaderLabels(names);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
    return t;
}
inline void addRow(QTableWidget* t, const QStringList& values) {
    int i = t->rowCount();
    t->insertRow(i);
    for (int j = 0; j < values.size(); ++j)
        t->setItem(i, j, new QTableWidgetItem(values[j]));
}
inline QString selected(QTableWidget* t, int col = 0) {
    auto* cell = t->item(t->currentRow(), col);
    return cell ? cell->text() : QString{};
}
inline QDoubleSpinBox* decimal(int precision, double maximum, double initial = 0) {
    auto* box = new QDoubleSpinBox;
    box->setLocale(QLocale::c());
    box->setDecimals(precision);
    box->setRange(0, maximum);
    box->setValue(initial);
    return box;
}
inline QDateEdit* calendar(QDate initial) {
    auto* w = new QDateEdit(initial);
    w->setCalendarPopup(true);
    w->setDisplayFormat("yyyy-MM-dd");
    return w;
}
inline void addWidgets(QHBoxLayout* row, std::initializer_list<QWidget*> widgets) {
    for (auto* widget : widgets)
        row->addWidget(widget);
}
