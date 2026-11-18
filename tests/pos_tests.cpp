#include "Checkout.h"
#include "TextStore.h"
#include "RecordCodec.h"
#include "Numbers.h"
#include "Receipt.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>

int checks = 0, failures = 0;
void check(bool passed, const QString& label)
{
    ++checks;
    if (!passed) { ++failures; std::cerr << "FAIL: " << label.toStdString() << '\n'; }
}
bool writeText(const QString& path, const QByteArray& text)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(text) == text.size();
}
QByteArray readText(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}
StoreState seed()
{
    StoreState s;
    s.products.append({"1", "123456", "MILK1L", "Fresh Milk 1 L", "count", "barcodes/milk label.png", 25000, 1000, 500, 24000});
    s.products.append({"2", "345678", "RICE", "Basmati Rice", "weight", "barcodes/rice.png", 30000, 1500, 1000, 10000});
    return s;
}
// A second Store implementation tests OCP and dependency injection without files.
class MemoryStore : public Store {
public:
    StoreState state = seed();
    bool failSave = false;
    bool load(StoreState& result, QString&) override { result = state; return true; }
    bool save(const StoreState& next, QString& error) override {
        if (failSave) { error = "Simulated disk failure."; return false; }
        state = next; return true;
    }
};
Order customer(const QString& status = "paid", qint64 amount = 66250)
{
    Order o;
    o.customer = "Ali Ahmed"; o.status = status; o.mode = "cash"; o.payment = amount;
    return o;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QString error;
    qint64 number = 0;
    check(parseFixed("250.05", 2, number) && number == 25005, "decimal money to paisa");
    check(parseFixed("0.125", 3, number) && number == 125, "fractional kilogram");
    for (const QString& bad : {"-1", "+1", "NaN", "1e4", "1.001", "1,000", " 1", "1 ", "1.", "", "1\n"})
        check(!parseFixed(bad, 2, number), "reject numeric text " + bad);
    check(!parseFixed("9999999999999999", 2, number), "numeric range bound");
    for (qint64 v : {0LL, 1LL, 99LL, 100LL, 10001LL, 123456789LL})
        check(parseFixed(fixedText(v, 2), 2, number) && number == v, "exact decimal round trip");
    check(lineAmount(1, 500) == 1 && lineAmount(1, 499) == 0, "half paisa rounds up");
    check(validQuantity("count", 2000) && !validQuantity("count", 1500), "whole count only");
    check(validQuantity("weight", 125) && !validQuantity("weight", 0), "positive fractional weight");
    check(!validQuantity("other", 1000) && !validQuantity("weight", -1), "unit and sign rejected");
    QStringList fields = {"Ali Ahmed", "Tea \"Gold\"", "Box #1", "", "C:/My Store/label.png"}, decoded;
    check(splitRecord(joinRecord(fields), decoded) && fields == decoded, "quoted field round trip");
    for (const QString& bad : {"one two", "one # extra", "\"unfinished #", "\"x\"bad #", "#", "\"a\nb\" #"})
        check(!splitRecord(bad, decoded), "reject malformed row " + bad);

    MemoryStore memory;
    Checkout checkout(memory);
    check(checkout.load(error), "load memory store");
    check(!checkout.add("999999", 1000, error), "unknown barcode");
    check(!checkout.add("123456", 0, error), "zero quantity");
    check(!checkout.add("123456", 1500, error), "fractional count");
    check(!checkout.add("123456", 24000, error), "equal stock rejected");
    check(!checkout.add("123456", 25000, error), "over stock rejected");
    check(checkout.add("123456", 23000, error), "below stock accepted");
    check(!checkout.add("123456", 1000, error) && checkout.items()[0].quantity == 23000, "combined quantity cannot equal stock");
    check(!checkout.load(error), "reload cannot replace active cart");
    check(!checkout.changeQuantity(-1, 1000, error), "missing row rejected");
    checkout.clear();
    check(checkout.add("123456", 1000, error) && checkout.add("123456", 1000, error), "repeated scans");
    check(checkout.items().size() == 1 && checkout.items()[0].quantity == 2000, "merge repeated product");
    check(checkout.add("345678", 500, error), "half kilogram");
    Totals total = checkout.totals();
    check(total.subtotal == 65000 && total.tax == 2750 && total.discount == 1500 && total.payable() == 66250, "golden multi-product bill");
    const QDateTime time(QDate(2026, 11, 19), QTime(10, 30, 0));
    Bill bill;
    Order invalid = customer(); invalid.customer = "  ";
    check(!checkout.issueBill(invalid, time, bill, error), "blank customer");
    invalid = customer(); invalid.customer = "Ali\nAhmed";
    check(!checkout.issueBill(invalid, time, bill, error), "newline injection");
    invalid = customer(); invalid.payment = 1;
    check(!checkout.issueBill(invalid, time, bill, error), "incorrect payment");
    invalid = customer(); invalid.mode = "card";
    check(!checkout.issueBill(invalid, time, bill, error), "card needs merchant");
    invalid = customer(); invalid.status = "archived";
    check(!checkout.issueBill(invalid, time, bill, error), "cannot initially archive");
    check(checkout.issueBill(customer(), time, bill, error), "issue paid bill");
    check(bill.id == "000000001" && bill.orderId == "00000119", "ID lengths and day suffix");
    check(checkout.items().isEmpty() && checkout.data().orders.size() == 1, "cart clears only after save");
    check(checkout.product("1")->available == 22000 && checkout.product("2")->available == 9500, "stock persisted once");
    check(checkout.data().orders[0].contact == "-" && checkout.data().orders[0].merchant == "-", "optional contact and cash marker");
    check(!checkout.issueBill(customer(), time, bill, error), "double submission blocked by empty cart");
    check(checkout.add("123456", 1000, error), "next order");
    check(checkout.issueBill(customer("on-hold", 0), time.addDays(1), bill, error), "hold order");
    check(bill.orderId == "00000220" && bill.id == "000000002", "ID sequence across days");
    check(!checkout.archive(bill.orderId, error), "unpaid order cannot archive");
    check(checkout.markPaid(bill.orderId, error), "settle held order");
    check(checkout.data().orders[1].payment == 25500 && checkout.product("1")->available == 21000, "payment update avoids second stock deduction");
    check(!checkout.markPaid(bill.orderId, error), "cannot pay twice");
    check(checkout.archive(bill.orderId, error) && checkout.data().bills[1].status == "archived", "archive updates order and bill");
    check(!checkout.archive(bill.orderId, error), "cannot archive twice");
    check(checkout.add("345678", 250, error), "quarter kilogram");
    Order wallet = customer("in-progress", 0); wallet.mode = "wallet"; wallet.merchant = "JazzCash";
    check(checkout.issueBill(wallet, time, bill, error), "in-progress wallet order");
    check(checkout.markPaid(bill.orderId, error), "settle in-progress order");
    check(!checkout.markPaid("12345678", error), "unknown saved order");

    MemoryStore failed;
    Checkout failCheckout(failed); failCheckout.load(error); failCheckout.add("123456", 1000, error);
    failed.failSave = true;
    check(!failCheckout.issueBill(customer("paid", 25500), time, bill, error), "failed save reports failure");
    check(failCheckout.isBlocked() && failCheckout.items().size() == 1 && failCheckout.data().orders.isEmpty(), "failed save retains cart and original state");
    check(!failCheckout.add("123456", 1000, error) && !failCheckout.load(error), "blocked session requires restart");

    QTemporaryDir temporary;
    check(temporary.isValid(), "temporary folder");
    TextStore disk(temporary.path());
    check(disk.save(memory.state, error), "write four files");
    StoreState restored;
    check(disk.load(restored, error), "read four files " + error);
    check(restored.products.size() == 2 && restored.orders.size() == 3 && restored.bills.size() == 3, "complete disk round trip");
    check(restored.products[0].imagePath == "barcodes/milk label.png", "path with spaces");
    QByteArray orderText = readText(temporary.filePath("Order.txt"));
    check(orderText.contains("\"Ali Ahmed\"") && orderText.count('\n') == 3, "one record per physical line");
    QByteArray productText = readText(temporary.filePath("Product.txt"));
    QByteArray billText = readText(temporary.filePath("Bill.txt"));
    QByteArray detailText = readText(temporary.filePath("OrderDetail.txt"));
    for (const QByteArray& line : billText.trimmed().split('\n'))
        check(line.endsWith('#') && splitRecord(QString::fromUtf8(line), decoded) && (decoded.size() - 8) % 4 == 0, "bill repeated group schema");
    check(!QFile::exists(temporary.filePath("Pending.json")), "journal removed after commit");
    writeText(temporary.filePath("Product.txt"), productText + productText);
    check(!disk.load(restored, error), "duplicate products rejected");
    check(restored.orders.size() == 3, "bad load preserves previous valid state");
    writeText(temporary.filePath("Product.txt"), productText);
    writeText(temporary.filePath("Order.txt"), orderText + "broken row\n");
    check(!disk.load(restored, error), "missing terminator rejected");
    writeText(temporary.filePath("Order.txt"), orderText);
    writeText(temporary.filePath("OrderDetail.txt"), detailText + "99999919 1 123456 1.000 #\n");
    check(!disk.load(restored, error), "orphan detail rejected");
    writeText(temporary.filePath("OrderDetail.txt"), detailText);
    QByteArray corruptBill = billText; corruptBill.replace("662.50", "662.51");
    writeText(temporary.filePath("Bill.txt"), corruptBill);
    check(!disk.load(restored, error), "incorrect bill totals rejected");
    writeText(temporary.filePath("Bill.txt"), billText);
    QByteArray newPrices = productText; newPrices.replace("250.00", "275.00");
    writeText(temporary.filePath("Product.txt"), newPrices);
    check(disk.load(restored, error) && restored.bills[0].totals.payable() == 66250, "historic bill keeps original price");
    Checkout restarted(disk);
    check(restarted.load(error) && restarted.add("123456", 1000, error), "restart uses new catalogue price");
    check(restarted.totals().payable() == 28000, "new order uses current price");
    check(restarted.issueBill(customer("paid", 28000), time, bill, error) && bill.id == "000000004", "IDs survive restart");

    QTemporaryDir recoveryDir;
    TextStore recovery(recoveryDir.path());
    StoreState before = seed(), after = seed(); after.products[0].available = 22000;
    check(recovery.save(before, error), "recovery fixture");
    QFile::remove(recoveryDir.filePath("Order.txt"));
    QDir().mkdir(recoveryDir.filePath("Order.txt"));
    check(!recovery.save(after, error) && QFile::exists(recoveryDir.filePath("Pending.json")), "interrupted multi-file write retains journal");
    QDir().rmdir(recoveryDir.filePath("Order.txt"));
    check(recovery.load(restored, error) && restored.products[0].available == 22000, "startup rolls forward pending save");
    check(recovery.load(restored, error) && !QFile::exists(recoveryDir.filePath("Pending.json")), "recovery is idempotent");
    writeText(recoveryDir.filePath("Pending.json"), "not json");
    check(!recovery.load(restored, error), "damaged journal stops safely");
    Bill escaped = memory.state.bills[0]; escaped.customer = "<script>";
    check(receiptHtml(escaped, memory.state.orders[0]).contains("&lt;script&gt;"), "receipt escapes user text");
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
