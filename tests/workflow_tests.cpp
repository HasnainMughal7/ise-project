#include "Checkout.h"
#include "TextStore.h"
#include "Reports.h"
#include "Receipt.h"
#include "ScanGate.h"
#include "TestKit.h"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Checks t;
    QTemporaryDir tmp;
    copySeed(argv[1], tmp.path());
    QString e, code, id;
    Access access(tmp.path());
    TextStore disk(tmp.path());
    Checkout s(disk, access);
    auto now = QDateTime(QDate(2026, 11, 23), QTime(12, 0));
    auto date = now.date();
    auto stock = [&](const QString& barcode) {
        return s.findCode(barcode)->available;
    };
    auto save = [&](QString status, qint64 received = 0, int rate = 0, QString mode = "cash") {
        return s.finish(status, "Demo Cashier", "", mode, mode == "cash" ? "-" : "Bank", rate, received,
                        now, id, e);
    };
    t.check(s.load(e), "seed read", e);
    if (t.failed)
        return 1;
    t.check(!access.setup("12", e) && access.setup("3469", e), "PIN setup", e);
    t.check(!s.addCategory({"45", "Pantry", "PAN"}, e), "cashier cannot manage");
    t.check(!access.unlock("1111") && access.unlock("3469"), "sign in");
    t.check(s.addCategory({"45", "Pantry", "PAN"}, e) && !s.addCategory({"45", "Other", "X"}, e),
            "unique category", e);
    t.check(!s.addCategory({"01", "Wrong", "X"}, e), "no leading zero");
    Product p;
    p.name = "Demo Oats";
    p.acronym = "OATS";
    p.unit = "count";
    p.category = "45";
    p.price = 10000;
    p.available = 11000;
    p.discount = 500;
    t.check(s.putProduct(p, code, e) && code == "450000", "prefix + first free suffix", e);
    p.name = "Oat drink";
    p.acronym = "OATDR";
    t.check(s.putProduct(p, code, e) && code == "450001", "second suffix unique");
    t.check(QFile::exists(tmp.filePath("barcodes/barcode_450001.png")), "PNG on disk");
    t.check(s.putSeason(-1, {date, date, 1000, true}, e), "season", e);
    t.check(s.putSeason(-1, {date, date.addDays(1), 500, true}, e), "overlap", e);
    t.check(s.putSeason(-1, {date, date, 8000, false}, e), "inactive season", e);
    t.check(activeRate(s.data().seasons, date) == 1000 &&
                activeRate(s.data().seasons, date.addDays(1)) == 500 &&
                !activeRate(s.data().seasons, date.addDays(-1)),
            "inclusive date, largest active rate");
    access.lock();
    t.check(!s.add("999999", 1000, e) && !s.add("123456", 500, e) && !s.add("123456", 24000, e),
            "unknown / fractional count / exact stock rejected");
    t.check(!save("paid"), "empty bill refused");
    t.check(s.add("123456", 2000, e) && s.add("131676", 1000, e) && save("on-hold"),
            "save original hold", e);
    const QString first = id, bill = s.findOrder(id)->billId;
    t.check(stock("123456") == 22000 && stock("131676") == 15000, "reserved stock");
    {
        TextStore again(tmp.path());
        Access same(tmp.path());
        Checkout fresh(again, same);
        t.check(fresh.load(e) && fresh.openOrder(first, "", now, e), "resume after restart", e);
        t.check(fresh.cart().lines.size() == 2 && fresh.cart().billId == bill,
                "line reconstruction / original bill");
        t.check(fresh.add("123456", 1000, e) && fresh.quantity(0, 2000, e) && fresh.remove(1) &&
                    fresh.add("345678", 500, e),
                "add, change, delete, fractional weight", e);
        t.check(fresh.finish("on-hold", "", "", "cash", "", 0, 0, now, id, e) && id == first,
                "rehold original ID", e);
    }
    t.check(s.load(e) && stock("131676") == 16000 && stock("123456") == 22000 &&
                stock("345678") == 9500,
            "only net stock difference applied", e);
    t.check(s.openOrder(first, "", now, e) && s.add("123456", 1000, e), "stage edit", e);
    s.discard();
    t.check(stock("123456") == 22000 && s.openOrder(first, "", now, e) &&
                s.cart().lines[0].quantity == 2000,
            "discard leaves saved hold");
    Totals z = calculate(s.priced(800, now));
    t.check(z.base == 65000 && z.items == 1500 && z.tax == 2750 && z.season == 6350 &&
                z.special == 4572 && z.payable() == 55328,
            "hand-calculated discount case");
    t.check(!save("paid", 50000, 800) && !save("paid", 60000, 801) && !save("paid", 60000, 800, "card"),
            "short tender / >8% / excess card rejected");
    t.check(save("paid", 60000, 800) && id == first && s.findOrder(id)->billId == bill,
            "pay original order with same bill", e);
    t.check(s.findOrder(id)->received - z.payable() == 4672 && stock("123456") == 22000,
            "change and no double deduction");
    const QString output = qEnvironmentVariable("ISE_DEMO_PDF", tmp.filePath("Example.pdf"));
    t.check(exportPdf(*s.findOrder(id), output, e) && bytes(output).startsWith("%PDF-"), "write PDF",
            e);
    t.check(!s.openOrder(first, "0000", now, e) && s.openOrder(first, "3469", now.addDays(7), e),
            "approval & inclusive seven day limit");
    t.check(!s.finish("paid", "", "", "cash", "", 800, 60000, now.addDays(7).addMSecs(1), id, e),
            "expiry rechecked at save");
    s.discard();
    t.check(!s.openOrder(first, "3469", now.addDays(7).addMSecs(1), e) &&
                !s.openOrder(first, "3469", now.addSecs(-1), e),
            "outside original payment window");
    t.check(s.openOrder(first, "3469", now.addDays(1), e) && s.quantity(0, 1000, e),
            "approved reduction");
    t.check(s.finish("paid", "Updated", "", "cash", "", 800, 60000, now.addDays(1), id, e),
            "save approved edit", e);
    t.check(stock("123456") == 23000 && s.findOrder(id)->paid == now && s.findOrder(id)->billed == now,
            "stock returned and date preserved");
    t.check(s.archive(id, e) && !s.openOrder(first, "3469", now, e), "archive read only");
    t.check(s.add("450000", 10000, e) && save("on-hold"), "ten items at five rupees", e);
    t.check(bytes(tmp.filePath("BillDetail.txt")).contains("OATS 100.00 10.000 50.00"),
            "line discount is extended");
    QString cancelled = id, previousBill = s.findOrder(id)->billId;
    t.check(s.cancelHeld(id, e) && stock("450000") == 11000 && !s.findOrder(cancelled),
            "cancel releases reservation", e);
    t.check(s.add("450000", 1000, e) && save("in-progress") && id != cancelled &&
                s.findOrder(id)->billId != previousBill,
            "ID high-water after cancellation");
    t.check(s.add("100003", 1000, e) && save("paid", 11400, 0, "card"), "card exact payment", e);
    auto rep = [&](ReportKind kind) {
        return makeReport(s.data(), kind, date, date);
    };
    t.check(rep(ReportKind::Incomplete).rows.size() == 1 && rep(ReportKind::Completed).rows.size() == 2,
            "open vs completed");
    t.check(rep(ReportKind::Cash).rows.size() == 1 &&
                makeReport(s.data(), ReportKind::Cash, date.addDays(1), date.addDays(1)).rows.isEmpty(),
            "cash filters");
    t.check(rep(ReportKind::Sales).rows.size() == 2 && rep(ReportKind::BestSellers).rows.size() == 3,
            "sales and ranking");
    t.check(rep(ReportKind::HighestStock).rows[0][1] == "Hand Soap", "highest free stock");
    t.check(periodRange(Period::Week, QDate(2026, 11, 29)).first == date &&
                periodRange(Period::Month, date).second.day() == 30 &&
                periodRange(Period::Year, date).first == QDate(2026, 1, 1),
            "period boundaries");
    access.unlock("3469");
    Product edit = *s.findCode("123456");
    edit.price += 1000;
    edit.category = "45";
    edit.acronym = "NEWMILK";
    t.check(s.putProduct(edit, code, e) && code.startsWith("45") && !s.findCode("123456"),
            "recategorize and relabel");
    t.check(s.load(e) && s.findOrder(first)->lines[0].barcode == "123456" &&
                s.findOrder(first)->lines[0].price == 25000,
            "history retains old code and price", e);
    for (int i = 0; i < 10; ++i) {
        p.name = "Extra " + QString::number(i);
        p.acronym = "EXT" + QString::number(i);
        p.available = (100 + i) * 1000;
        t.check(s.putProduct(p, code, e), "additional ranked product", e);
    }
    auto r = rep(ReportKind::HighestStock);
    t.check(r.rows.size() == 10 && r.rows[0][1] == "Extra 9" && r.rows[9][1] == "Extra 0",
            "ranking descending and capped");
    ScanGate g;
    t.check(g.observe("123456").isEmpty() && g.observe("123456") == "123456" &&
                g.observe("123456").isEmpty(),
            "one event for stationary label");
    g.observe("");
    g.observe("");
    g.observe("");
    g.observe("123456");
    t.check(g.observe("123456") == "123456", "same label rearms after removal");
    g.observe("345678");
    t.check(g.observe("345678") == "345678", "new label without button");
    return t.result();
}
