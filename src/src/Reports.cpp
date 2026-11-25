#include "Reports.h"
#include "Numbers.h"
#include <QMap>
#include <algorithm>

QPair<QDate, QDate> periodRange(Period p, QDate day) {
    if (p == Period::Week) {
        QDate first = day.addDays(1 - day.dayOfWeek());
        return {first, first.addDays(6)};
    }
    if (p == Period::Month)
        return {QDate(day.year(), day.month(), 1), QDate(day.year(), day.month(), day.daysInMonth())};
    if (p == Period::Year)
        return {QDate(day.year(), 1, 1), QDate(day.year(), 12, 31)};
    return {day, day};
}
Report makeReport(const StoreState& s, ReportKind k, QDate from, QDate through) {
    Report r;
    if (k == ReportKind::HighestStock || k == ReportKind::BestSellers) {
        QMap<QString, qint64> sold;
        for (const Order& o : s.orders)
            if (settled(o))
                for (const Line& l : o.lines)
                    sold[l.productId] += l.quantity;
        QVector<Product> ranked = s.products;
        auto score = [&](const Product& p) {
            return k == ReportKind::BestSellers ? sold.value(p.id) : p.available;
        };
        std::sort(ranked.begin(), ranked.end(), [&](const Product& a, const Product& b) {
            if (score(a) != score(b))
                return score(a) > score(b);
            return a.id < b.id;
        });
        r.columns = {"ID", "Product", "Unit", "Sold", "Remaining"};
        for (const Product& p : ranked) {
            if (k == ReportKind::BestSellers && !sold.value(p.id))
                continue;
            r.rows.append(
                {p.id, p.name, p.unit, fixedText(sold.value(p.id), 3), fixedText(p.available, 3)});
            if (r.rows.size() == 10)
                break;
        }
        r.summary = QString("%1 products shown. Weight is kg.").arg(r.rows.size());
        return r;
    }
    r.columns = {"Order ID", "Bill ID", "Customer",    "Date",
                 "Status",   "Mode",    "Payable PKR", "Discount PKR"};
    qint64 amount = 0, discounts = 0;
    for (const Order& o : s.orders) {
        bool paid = settled(o);
        if ((k == ReportKind::Incomplete && paid) || (k != ReportKind::Incomplete && !paid))
            continue;
        QDate date = (paid ? o.paid : o.opened).toLocalTime().date();
        if (k == ReportKind::Cash && (o.mode != "cash" || date < from || date > through))
            continue;
        Totals t = calculate(o);
        r.rows.append({o.id, o.billId, o.customer, date.toString(Qt::ISODate), o.status, o.mode,
                       fixedText(t.payable(), 2), fixedText(t.discounts(), 2)});
        amount += t.payable();
        discounts += t.discounts();
    }
    r.summary = QString("%1 orders    Total PKR %2    Discounts PKR %3")
                    .arg(r.rows.size())
                    .arg(fixedText(amount, 2), fixedText(discounts, 2));
    return r;
}
