#pragma once
#include "Store.h"
#include <QDateTime>

class Checkout {
public:
    explicit Checkout(Store& store);
    bool load(QString& error);
    const StoreState& data() const { return state; }
    const QVector<OrderLine>& items() const { return cart; }
    const Product* product(const QString& id) const;
    const Product* findBarcode(const QString& barcode) const;
    bool add(const QString& barcode, qint64 quantity, QString& error);
    bool changeQuantity(int row, qint64 quantity, QString& error);
    void remove(int row);
    void clear();
    Totals totals() const;
    bool issueBill(Order customer, const QDateTime& when, Bill& bill, QString& error);
    bool markPaid(const QString& orderId, QString& error);
    bool archive(const QString& orderId, QString& error);
    bool isBlocked() const { return blocked; }
private:
    Store& store;
    StoreState state;
    QVector<OrderLine> cart;
    bool blocked = false;
    bool saveState(const StoreState& next, QString& error);
    bool checkQuantity(const Product& product, qint64 quantity, QString& error) const;
    bool updateStatus(const QString& orderId, bool paid, QString& error);
};
