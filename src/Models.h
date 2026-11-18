#pragma once
#include <QString>
#include <QVector>

// Money uses paisa; quantity uses thousandths of an item or kilogram.
struct Product {
    QString id, barcode, acronym, name, unit, imagePath;
    qint64 price = 0, tax = 0, discount = 0, available = 0;
};
struct OrderLine {
    QString productId, barcode;
    qint64 quantity = 0;
};
struct Order {
    QString id, date, time, customer, contact, status, mode, merchant;
    qint64 payment = 0;
    QVector<OrderLine> lines;
};
struct BillLine {
    QString acronym;
    qint64 price = 0, discount = 0, quantity = 0;
};
struct Totals {
    qint64 subtotal = 0, tax = 0, discount = 0;
    qint64 payable() const { return subtotal + tax - discount; }
};
struct Bill {
    QString id, customer, orderId, status;
    QVector<BillLine> lines;
    Totals totals;
};
struct StoreState {
    QVector<Product> products;
    QVector<Order> orders;
    QVector<Bill> bills;
};
