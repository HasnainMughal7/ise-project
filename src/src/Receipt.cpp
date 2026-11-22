#include "Receipt.h"
#include "Numbers.h"

QString receiptHtml(const Bill& b, const Order& o)
{
    QString html = "<h1>Corner Store</h1><h2>Bill " + b.id.toHtmlEscaped() + "</h2>";
    html += "<p>Order " + o.id.toHtmlEscaped() + " &nbsp; " + o.date.toHtmlEscaped() + " " + o.time.toHtmlEscaped() + "</p>";
    html += "<p>Customer: " + b.customer.toHtmlEscaped() + "<br>Contact: " + o.contact.toHtmlEscaped() + "</p>";
    html += "<table width='100%' cellpadding='8'><tr bgcolor='#e5edf3'><th>Product</th><th>Unit PKR</th><th>Quantity</th><th>Discount/unit</th><th>Base PKR</th></tr>";
    for (int i = 0; i < b.lines.size(); ++i) {
        const BillLine& line = b.lines[i];
        QString item = line.acronym.toHtmlEscaped();
        if (i < o.lines.size()) item += "<br>ID " + o.lines[i].productId.toHtmlEscaped() + " / " + o.lines[i].barcode.toHtmlEscaped();
        html += "<tr><td>" + item + "</td><td>" + fixedText(line.price, 2) +
            "</td><td>" + fixedText(line.quantity, 3) + "</td><td>" + fixedText(line.discount, 2) +
            "</td><td>" + fixedText(lineAmount(line.price, line.quantity), 2) + "</td></tr>";
    }
    html += "</table><p>Subtotal before tax and discount: PKR " + fixedText(b.totals.subtotal, 2) +
        "<br>Tax: PKR " + fixedText(b.totals.tax, 2) + "<br>Discount: PKR " + fixedText(b.totals.discount, 2) + "</p>";
    html += "<h2>Total payable: PKR " + fixedText(b.totals.payable(), 2) + "</h2>";
    html += "<p>Status: " + b.status.toHtmlEscaped() + "<br>Payment recorded: PKR " + fixedText(o.payment, 2) +
        "<br>Mode: " + o.mode.toHtmlEscaped() + "<br>Merchant: " + o.merchant.toHtmlEscaped() + "</p>";
    html += "<p>Weight quantities are in kilograms. Payment is recorded locally.</p>";
    return html;
}

QString productDescription(const Product& p)
{
    return "Product ID: " + p.id + "\nBarcode: " + p.barcode + "\nAcronym: " + p.acronym +
        "\nName: " + p.name + "\nUnit: " + p.unit + (p.unit == "weight" ? " (kg)" : " (items)") +
        "\nUnit price: PKR " + fixedText(p.price, 2) + "\nTax per unit: PKR " + fixedText(p.tax, 2) +
        "\nDiscount per unit: PKR " + fixedText(p.discount, 2) +
        "\nAvailable quantity: " + fixedText(p.available, 3) + "\nBarcode image: " + p.imagePath;
}
