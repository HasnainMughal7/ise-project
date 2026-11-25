#include "Models.h"
#include "Numbers.h"
#include "Barcode.h"

bool settled(const Order& o) {
    return o.status == "paid" || o.status == "archived";
}
bool cleanText(const QString& s, int limit) {
    if (s.trimmed().isEmpty() || s.size() > limit)
        return false;
    for (QChar c : s)
        if (c.isSpace() && c != ' ')
            return false;
    for (QChar c : s)
        if (!c.isPrint())
            return false;
    return true;
}
bool validProduct(const Product& p) {
    return cleanText(p.id) && cleanText(p.name) && cleanText(p.acronym, 8) &&
           (p.unit == "count" || p.unit == "weight") && p.price >= 0 && p.price <= 100000000 &&
           p.tax >= 0 && p.tax <= 100000000 && p.discount >= 0 && p.discount <= p.price &&
           p.available >= 0 && p.available <= 1000000000 &&
           (p.unit != "count" || p.available % 1000 == 0) && validateNumber(p.barcode).isEmpty() &&
           p.category == p.barcode.left(2);
}
int activeRate(const QVector<Season>& seasons, QDate date) {
    int rate = 0;
    for (const Season& s : seasons)
        if (s.active && s.start <= date && date <= s.end)
            rate = qMax(rate, s.rate);
    return rate;
}
static qint64 percentage(qint64 amount, int basisPoints) {
    return (amount / 10000) * basisPoints + ((amount % 10000) * basisPoints + 5000) / 10000;
}
Totals calculate(const Order& o) {
    Totals t;
    for (const Line& l : o.lines) {
        t.base += lineAmount(l.price, l.quantity);
        t.tax += lineAmount(l.tax, l.quantity);
        t.items += lineAmount(l.discount, l.quantity);
    }
    if (o.legacyTax >= 0)
        t.tax = o.legacyTax;
    qint64 merchandise = t.base - t.items;
    t.season = percentage(merchandise, o.seasonRate);
    t.special = percentage(merchandise - t.season, o.specialRate);
    return t;
}
qint64 quantityOf(const QVector<Line>& lines, const QString& id) {
    for (const Line& l : lines)
        if (l.productId == id)
            return l.quantity;
    return 0;
}
