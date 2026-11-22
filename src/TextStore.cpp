#include "TextStore.h"
#include "Barcode.h"
#include "Numbers.h"
#include "RecordCodec.h"
#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTime>

static const QStringList fileNames = {"Product.txt", "Order.txt", "OrderDetail.txt", "Bill.txt"};

TextStore::TextStore(const QString& path) : directory(path) {}

bool TextStore::readRows(const QString& name, QVector<QStringList>& rows, QString& error)
{
    QFile file(QDir(directory).filePath(name));
    if (!file.open(QIODevice::ReadOnly) || file.size() > 10000000) {
        error = "Cannot read " + name + ". Check the file and its size (maximum 10 MB).";
        return false;
    }
    int lineNumber = 0;
    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());
        ++lineNumber;
        if (lineNumber == 1 && line.startsWith(QChar(0xfeff))) line.remove(0, 1);
        if (line.trimmed().isEmpty()) continue;
        QStringList fields;
        if (!splitRecord(line, fields) || rows.size() >= 10000) {
            error = name + ", line " + QString::number(lineNumber) + ": invalid record or too many records.";
            return false;
        }
        rows.append(fields);
    }
    if (file.error() != QFile::NoError) { error = "Read failed for " + name + ": " + file.errorString(); return false; }
    return true;
}

bool TextStore::load(StoreState& state, QString& error)
{
    error.clear();
    if (!recover(error)) return false;
    QVector<QStringList> products, orders, details, bills;
    if (!readRows(fileNames[0], products, error) || !readRows(fileNames[1], orders, error) ||
        !readRows(fileNames[2], details, error) || !readRows(fileNames[3], bills, error)) return false;
    StoreState next;
    QMap<QString, int> productIndex, orderIndex;
    QSet<QString> barcodes, billIds, billedOrders;
    for (const QStringList& f : products) {
        if (f.size() != 10) { error = "Product.txt: each row needs 10 fields."; return false; }
        Product p;
        p.id = f[0]; p.barcode = f[1]; p.acronym = f[2]; p.name = f[3]; p.unit = f[4]; p.imagePath = f[9];
        if (p.id.isEmpty() || productIndex.contains(p.id) || !validateNumber(p.barcode).isEmpty() ||
            barcodes.contains(p.barcode) || p.acronym.isEmpty() || p.acronym.size() > 8 || p.name.isEmpty() ||
            (p.unit != "count" && p.unit != "weight") || p.imagePath.isEmpty() ||
            !parseFixed(f[5], 2, p.price) || !parseFixed(f[6], 2, p.tax) ||
            !parseFixed(f[7], 2, p.discount) || !parseFixed(f[8], 3, p.available) ||
            p.price > 100000000 || p.tax > 100000000 || p.discount > p.price ||
            p.available > 1000000000 || (p.unit == "count" && p.available % 1000 != 0)) {
            error = "Product.txt: invalid or duplicate product " + p.id + "."; return false;
        }
        productIndex[p.id] = next.products.size();
        barcodes.insert(p.barcode);
        next.products.append(p);
    }
    const QStringList statuses = {"in-progress", "on-hold", "paid", "archived"};
    const QStringList modes = {"cash", "card", "wallet"};
    for (const QStringList& f : orders) {
        if (f.size() != 9) { error = "Order.txt: each row needs 9 fields."; return false; }
        Order o;
        o.id = f[0]; o.date = f[1]; o.time = f[2]; o.customer = f[3]; o.contact = f[4];
        o.status = f[5]; o.mode = f[7]; o.merchant = f[8];
        QDate date = QDate::fromString(o.date, Qt::ISODate);
        QTime time = QTime::fromString(o.time, "HH:mm:ss");
        if (!QRegularExpression("^[0-9]{8}$").match(o.id).hasMatch() || orderIndex.contains(o.id) ||
            !date.isValid() || date.toString(Qt::ISODate) != o.date || o.id.right(2) != date.toString("dd") ||
            !time.isValid() || time.toString("HH:mm:ss") != o.time || o.customer.isEmpty() ||
            !statuses.contains(o.status) || !modes.contains(o.mode) || o.merchant.isEmpty() ||
            (o.mode != "cash" && o.merchant == "-") || !parseFixed(f[6], 2, o.payment)) {
            error = "Order.txt: invalid or duplicate order " + o.id + "."; return false;
        }
        orderIndex[o.id] = next.orders.size();
        next.orders.append(o);
    }
    QSet<QString> detailKeys;
    for (const QStringList& f : details) {
        if (f.size() != 4 || !orderIndex.contains(f[0]) || !productIndex.contains(f[1])) {
            error = "OrderDetail.txt: invalid fields or missing order/product."; return false;
        }
        const Product& p = next.products[productIndex[f[1]]];
        OrderLine line{f[1], f[2], 0};
        QString key = f[0] + '\n' + f[1];
        if (p.barcode != line.barcode || !parseFixed(f[3], 3, line.quantity) ||
            !validQuantity(p.unit, line.quantity) || detailKeys.contains(key)) {
            error = "OrderDetail.txt: invalid or duplicate line for " + f[0] + "."; return false;
        }
        detailKeys.insert(key);
        next.orders[orderIndex[f[0]]].lines.append(line);
    }
    for (const QStringList& f : bills) {
        if (f.size() < 12 || f.size() > 408 || (f.size() - 8) % 4 != 0) {
            error = "Bill.txt: expected header, product groups and five final fields."; return false;
        }
        Bill b;
        b.id = f[0]; b.customer = f[1]; b.orderId = f[2]; b.status = f.last();
        if (!QRegularExpression("^[0-9]{9}$").match(b.id).hasMatch() || billIds.contains(b.id) ||
            !orderIndex.contains(b.orderId) || billedOrders.contains(b.orderId)) {
            error = "Bill.txt: invalid bill ID or order reference."; return false;
        }
        qint64 subtotal = 0, discount = 0;
        for (int i = 3; i < f.size() - 5; i += 4) {
            BillLine line;
            line.acronym = f[i];
            if (line.acronym.isEmpty() || line.acronym.size() > 8 ||
                !parseFixed(f[i + 1], 2, line.price) || !parseFixed(f[i + 2], 2, line.discount) ||
                !parseFixed(f[i + 3], 3, line.quantity) || line.price > 100000000 ||
                line.discount > line.price || line.quantity <= 0 || line.quantity > 1000000000) {
                error = "Bill.txt: invalid product group."; return false;
            }
            subtotal += lineAmount(line.price, line.quantity);
            discount += lineAmount(line.discount, line.quantity);
            b.lines.append(line);
        }
        int end = f.size() - 5;
        qint64 payable = 0;
        const Order& o = next.orders[orderIndex[b.orderId]];
        if (!parseFixed(f[end], 2, b.totals.subtotal) || !parseFixed(f[end + 1], 2, b.totals.tax) ||
            !parseFixed(f[end + 2], 2, b.totals.discount) || !parseFixed(f[end + 3], 2, payable) ||
            subtotal != b.totals.subtotal || discount != b.totals.discount || payable != b.totals.payable() ||
            b.customer != o.customer || b.status != o.status || b.lines.size() != o.lines.size() ||
            ((o.status == "paid" || o.status == "archived") ? o.payment != payable : o.payment != 0)) {
            error = "Bill.txt: totals, payment or order details do not match."; return false;
        }
        for (int i = 0; i < b.lines.size(); ++i) {
            if (b.lines[i].quantity != o.lines[i].quantity) {
                error = "Bill.txt: quantity differs from OrderDetail.txt."; return false;
            }
        }
        billIds.insert(b.id);
        billedOrders.insert(b.orderId);
        next.bills.append(b);
    }
    if (next.orders.size() != next.bills.size()) {
        error = "Every saved order must have exactly one bill."; return false;
    }
    state = next;
    return true;
}

QMap<QString, QByteArray> TextStore::serialize(const StoreState& s) const
{
    QMap<QString, QByteArray> files;
    for (const QString& name : fileNames) files[name] = {};
    for (const Product& p : s.products)
        files["Product.txt"] += joinRecord({p.id, p.barcode, p.acronym, p.name, p.unit,
            fixedText(p.price, 2), fixedText(p.tax, 2), fixedText(p.discount, 2),
            fixedText(p.available, 3), p.imagePath}).toUtf8();
    for (const Order& o : s.orders) {
        files["Order.txt"] += joinRecord({o.id, o.date, o.time, o.customer, o.contact, o.status,
            fixedText(o.payment, 2), o.mode, o.merchant}).toUtf8();
        for (const OrderLine& l : o.lines)
            files["OrderDetail.txt"] += joinRecord({o.id, l.productId, l.barcode, fixedText(l.quantity, 3)}).toUtf8();
    }
    for (const Bill& b : s.bills) {
        QStringList f = {b.id, b.customer, b.orderId};
        for (const BillLine& l : b.lines)
            f << l.acronym << fixedText(l.price, 2) << fixedText(l.discount, 2) << fixedText(l.quantity, 3);
        f << fixedText(b.totals.subtotal, 2) << fixedText(b.totals.tax, 2)
          << fixedText(b.totals.discount, 2) << fixedText(b.totals.payable(), 2) << b.status;
        files["Bill.txt"] += joinRecord(f).toUtf8();
    }
    return files;
}

bool TextStore::writeFile(const QString& name, const QByteArray& bytes, QString& error)
{
    QSaveFile file(QDir(directory).filePath(name));
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = "Cannot save " + name + ": " + file.errorString();
        return false;
    }
    return true;
}

bool TextStore::recover(QString& error)
{
    QFile journal(QDir(directory).filePath("Pending.json"));
    if (!journal.exists()) return true;
    if (!journal.open(QIODevice::ReadOnly) || journal.size() > 50000000) {
        error = "Cannot open pending save. Keep Pending.json and restore write access."; return false;
    }
    QJsonDocument document = QJsonDocument::fromJson(journal.readAll());
    journal.close();
    QJsonObject files = document.object();
    if (files.size() != 4) { error = "Pending.json is damaged. Restore the complete data backup."; return false; }
    for (const QString& name : fileNames) {
        if (!files.value(name).isString()) { error = "Invalid pending save structure."; return false; }
    }
    for (const QString& name : fileNames)
        if (!writeFile(name, files[name].toString().toUtf8(), error)) return false;
    if (!journal.remove()) { error = "Cannot finish pending save. Restore write access and restart."; return false; }
    return true;
}

bool TextStore::save(const StoreState& state, QString& error)
{
    int detailCount = 0;
    for (const Order& o : state.orders) detailCount += o.lines.size();
    if (state.products.size() > 10000 || state.orders.size() > 10000 || detailCount > 10000 ||
        QFile::exists(QDir(directory).filePath("Pending.json"))) {
        error = "Store capacity reached or an earlier save needs recovery."; return false;
    }
    // The after-image is written first. Interrupted writes roll forward at startup.
    QJsonObject journal;
    const auto files = serialize(state);
    for (auto i = files.begin(); i != files.end(); ++i) {
        if (i.value().size() > 10000000) { error = "Store file size limit reached."; return false; }
        journal.insert(i.key(), QString::fromUtf8(i.value()));
    }
    if (!writeFile("Pending.json", QJsonDocument(journal).toJson(), error)) return false;
    return recover(error);
}
