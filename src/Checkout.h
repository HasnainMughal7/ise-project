#pragma once
#include "Access.h"
#include "Store.h"

class Checkout {
  public:
    Checkout(Store& store, Access& access);
    bool load(QString& error);
    const StoreState& data() const {
        return state;
    }
    const Order& cart() const {
        return draft;
    }
    const Order* findOrder(const QString& id) const;
    const Product* findCode(const QString& code) const;
    bool openOrder(const QString& id, const QString& approvalPin, QDateTime now, QString& error);
    void discard();
    bool add(const QString& code, qint64 quantity, QString& error);
    bool quantity(int row, qint64 value, QString& error);
    bool remove(int row);
    Order priced(int specialRate, QDateTime now) const;
    bool finish(const QString& status, QString customer, QString contact, QString mode,
                QString merchant, int specialRate, qint64 received, QDateTime now, QString& orderId,
                QString& error);
    bool cancelHeld(const QString& id, QString& error);
    bool archive(const QString& id, QString& error);
    bool addCategory(Category item, QString& error);
    bool putProduct(Product item, QString& outCode, QString& error);
    bool setStock(const QString& id, qint64 available, QString& error);
    bool putSeason(int row, Season season, QString& error);
    bool blocked() const {
        return writeBlocked;
    }

  private:
    Store& store;
    Access& access;
    StoreState state;
    Order draft;
    QString approvedId;
    bool writeBlocked = false;
    bool publish(const StoreState& next, QString& error);
    bool checkStock(const Product& p, qint64 desired, QString& error) const;
    bool managerOnly(QString& error) const;
};
