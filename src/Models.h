#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>

struct Category {
    QString id, name, acronym;
};
struct Product {
    QString id, barcode, acronym, name, unit, imagePath, category;
    qint64 price = 0, tax = 0, discount = 0, available = 0;
};
struct Season {
    QDate start, end;
    int rate = 0;
    bool active = false;
};
// Quantities use thousandths. Money uses paisa. Percentage rates use hundredths of one percent.
struct Line {
    QString productId, barcode, acronym, unit;
    qint64 quantity = 0, price = 0, tax = 0, discount = 0;
};
struct Order {
    QString id, billId, customer = "Walk-in", contact = "-", status = "in-progress";
    QString mode = "cash", merchant = "-";
    QDateTime opened, billed, paid;
    int seasonRate = 0, specialRate = 0;
    qint64 received = 0, legacyTax = -1;
    QVector<Line> lines;
};
struct Totals {
    qint64 base = 0, tax = 0, items = 0, season = 0, special = 0;
    qint64 discounts() const {
        return items + season + special;
    }
    qint64 payable() const {
        return base + tax - discounts();
    }
};
struct StoreState {
    QVector<Category> categories;
    QVector<Product> products;
    QVector<Season> seasons;
    QVector<Order> orders;
    int lastOrder = 0, lastBill = 0;
};
bool settled(const Order& order);
bool cleanText(const QString& value, int limit = 100);
bool validProduct(const Product& product);
int activeRate(const QVector<Season>& seasons, QDate date);
Totals calculate(const Order& order);
qint64 quantityOf(const QVector<Line>& lines, const QString& productId);
