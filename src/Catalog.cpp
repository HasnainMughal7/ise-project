#include "Checkout.h"
#include <QRegularExpression>
#include <QSet>

bool Checkout::managerOnly(QString& error) const {
    if (access.isManager())
        return true;
    error = "Unlock the manager workspace first.";
    return false;
}
bool Checkout::addCategory(Category c, QString& error) {
    if (!managerOnly(error))
        return false;
    c.name = c.name.trimmed();
    c.acronym = c.acronym.trimmed().toUpper();
    if (!QRegularExpression("\\A[1-9][0-9]\\z").match(c.id).hasMatch() || !cleanText(c.name) ||
        !cleanText(c.acronym, 8)) {
        error = "Use a unique ID from 10 to 99, a name and a short acronym.";
        return false;
    }
    for (const Category& old : state.categories)
        if (old.id == c.id || old.acronym.compare(c.acronym, Qt::CaseInsensitive) == 0) {
            error = "Duplicate category ID or acronym.";
            return false;
        }
    StoreState next = state;
    next.categories.append(c);
    return publish(next, error);
}
bool Checkout::putProduct(Product p, QString& outCode, QString& error) {
    if (!managerOnly(error))
        return false;
    bool categoryFound = false;
    for (const Category& c : state.categories)
        if (c.id == p.category)
            categoryFound = true;
    if (!categoryFound) {
        error = "Create or choose a category first.";
        return false;
    }
    StoreState next = state;
    int row = -1;
    QSet<QString> productIds, codes;
    for (int i = 0; i < state.products.size(); ++i) {
        const Product& old = state.products[i];
        productIds.insert(old.id);
        codes.insert(old.barcode);
        if (!p.id.isEmpty() && old.id == p.id)
            row = i;
    }
    for (const Order& o : state.orders)
        for (const Line& l : o.lines)
            codes.insert(l.barcode);
    if (!p.id.isEmpty() && row < 0) {
        error = "Product ID was not found.";
        return false;
    }
    if (row < 0) {
        int serial = 1;
        while (productIds.contains(QString::number(serial)))
            ++serial;
        p.id = QString::number(serial);
    } else if (p.unit != state.products[row].unit) {
        for (const Order& o : state.orders)
            if (quantityOf(o.lines, p.id) > 0) {
                error = "A product with order history keeps its measurement unit.";
                return false;
            }
    }
    if (row >= 0 && state.products[row].category == p.category)
        p.barcode = state.products[row].barcode;
    else {
        p.barcode.clear();
        for (int suffix = 0; suffix < 10000; ++suffix) {
            QString code = p.category + QString("%1").arg(suffix, 4, 10, QChar('0'));
            if (!codes.contains(code)) {
                p.barcode = code;
                break;
            }
        }
        if (p.barcode.isEmpty()) {
            error = "All suffixes in this category are used.";
            return false;
        }
    }
    p.name = p.name.trimmed();
    p.acronym = p.acronym.trimmed().toUpper();
    p.imagePath = "barcodes/barcode_" + p.barcode + ".png";
    if (!validProduct(p)) {
        error = "Check the name, acronym (1–8), stock, unit and nonnegative prices. Discount cannot "
                "exceed price.";
        return false;
    }
    if (row < 0)
        next.products.append(p);
    else
        next.products[row] = p;
    if (!publish(next, error))
        return false;
    outCode = p.barcode;
    return true;
}
bool Checkout::setStock(const QString& id, qint64 value, QString& error) {
    if (!managerOnly(error))
        return false;
    for (const Product& p : state.products)
        if (p.id == id) {
            Product copy = p;
            copy.available = value;
            QString code;
            return putProduct(copy, code, error);
        }
    error = "Select a product.";
    return false;
}
bool Checkout::putSeason(int row, Season s, QString& error) {
    if (!managerOnly(error))
        return false;
    if (!s.start.isValid() || !s.end.isValid() || s.end < s.start || s.rate <= 0 || s.rate > 10000 ||
        row < -1 || row >= state.seasons.size()) {
        error = "Check season dates and rate (0.01% to 100%).";
        return false;
    }
    StoreState next = state;
    if (row < 0)
        next.seasons.append(s);
    else
        next.seasons[row] = s;
    return publish(next, error);
}
