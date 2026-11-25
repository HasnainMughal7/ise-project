#include "Receipt.h"
#include "Numbers.h"
#include <QPdfWriter>
#include <QTextDocument>
#include <QSaveFile>

QString receiptHtml(const Order& o) {
    Totals t = calculate(o);
    auto text = [](const QString& v) {
        return v.toHtmlEscaped();
    };
    auto money = [](qint64 n) {
        return fixedText(n, 2);
    };
    QString h = "<h1>Corner Store</h1><h2>Bill " + text(o.billId) + "</h2><p>Order " + text(o.id) +
                " &nbsp; " + text(o.billed.toLocalTime().toString("yyyy-MM-dd HH:mm:ss")) + "<br>" +
                text(o.customer) + " &nbsp; " + text(o.contact) + "<br>" + text(o.status) + " &nbsp; " +
                text(o.mode) + " &nbsp; " + text(o.merchant) + "</p>";
    h += "<table cellspacing='0' cellpadding='5' width='100%'><tr bgcolor='#dcefe9'><th>Item / "
         "code</th><th>Unit PKR</th><th>Quantity</th><th>Line discount</th><th>Base PKR</th></tr>";
    for (const Line& l : o.lines)
        h += "<tr><td>" + text(l.acronym) + "<br>" + text(l.barcode) + "</td><td>" + money(l.price) +
             "</td><td>" + fixedText(l.quantity, 3) + " " + text(l.unit == "weight" ? "kg" : "pcs") +
             "</td><td>" + money(lineAmount(l.discount, l.quantity)) + "</td><td>" +
             money(lineAmount(l.price, l.quantity)) + "</td></tr>";
    h += "</table><p>Subtotal before tax and discounts: " + money(t.base) +
         "<br>Item discounts: " + money(t.items) + "<br>Seasonal discount (" +
         fixedText(o.seasonRate, 2) + "%): " + money(t.season) + "<br>Special discount (" +
         fixedText(o.specialRate, 2) + "%): " + money(t.special) + "<br>Tax: " + money(t.tax) +
         "</p><h2>Total payable: PKR " + money(t.payable()) + "</h2>";
    return h + "<p>Amount received: " + money(o.received) +
           "<br>Balance returned: " + money(settled(o) ? o.received - t.payable() : 0) + "</p>";
}
bool exportPdf(const Order& o, const QString& path, QString& error) {
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) {
        error = out.errorString();
        return false;
    }
    {
        QPdfWriter pdf(&out);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setResolution(96);
        pdf.setTitle("Corner Store bill " + o.billId);
        QTextDocument doc;
        doc.setHtml(receiptHtml(o));
        doc.print(&pdf);
    }
    if (out.size() < 100 || !out.commit()) {
        error = "PDF save failed: " + out.errorString();
        return false;
    }
    return true;
}
