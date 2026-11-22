#include "MainWindow.h"
#include "ScanDialog.h"
#include "TextStore.h"
#include "Barcode.h"
#include "ImageReader.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>

int checks = 0, failures = 0;
void check(bool ok, const char* name) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    if (argc < 2) return 1;
    QString source = argv[1];
    QString screenshots = argc > 2 ? argv[2] : "";
    auto capture = [&](QWidget& w, const QString& name) {
        QApplication::processEvents();
        if (!screenshots.isEmpty()) w.grab().save(QDir(screenshots).filePath(name + ".png"));
    };
    QTemporaryDir temp;
    for (const QString& name : {"Product.txt", "Order.txt", "OrderDetail.txt", "Bill.txt"})
        check(QFile::copy(QDir(source).filePath(name), temp.filePath(name)), "copy seed file");
    TextStore store(temp.path());
    Checkout checkout(store);
    QString error;
    check(checkout.load(error), "load demo catalogue");
    for (const Product& product : checkout.data().products) {
        QImage label = makeBarcode(product.barcode);
        check(readBarcode(label).number == product.barcode, "generated demo label reads");
        // Optional output is used only by the package builder, never by CTest.
        if (argc > 3) check(label.save(QDir(argv[3]).filePath("barcode_" + product.barcode + ".png")), "write demo label");
    }
    check(checkout.add("123456", 2000, error) && checkout.add("345678", 500, error), "seed two item cart");
    MainWindow window(checkout, temp.path());
    window.show();
    auto tabs = window.findChild<QTabWidget*>("tabs");
    auto customer = window.findChild<QLineEdit*>("customer");
    auto payment = window.findChild<QLineEdit*>("payment");
    customer->setText("Ali Ahmed");
    check(window.findChild<QTableWidget*>("cartTable")->rowCount() == 2, "cart shows both products");
    check(payment->text() == "662.50", "exact amount displayed");
    capture(window, "checkout");
    window.resize(1020, 720); capture(window, "checkout_small");
    window.resize(1180, 810);
    QTimer::singleShot(100, [&]() {
        auto dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        check(dialog && dialog->windowTitle().startsWith("Bill "), "bill dialog opens");
        if (dialog) { capture(*dialog, "receipt"); dialog->accept(); }
    });
    window.findChild<QPushButton*>("issueButton")->click();
    check(checkout.items().isEmpty() && checkout.data().orders.size() == 1, "GUI bill saves and clears cart");
    check(!window.findChild<QPushButton*>("issueButton")->isEnabled(), "double billing button disabled");
    StoreState reload;
    check(store.load(reload, error) && reload.bills[0].totals.payable() == 66250, "GUI saved files reload");
    tabs->setCurrentIndex(1); capture(window, "products");
    tabs->setCurrentIndex(2); capture(window, "orders");
    check(window.findChild<QTableWidget*>("ordersTable")->rowCount() == 1, "saved order visible");
    tabs->setCurrentIndex(3);
    auto code = window.findChild<QLineEdit*>("barcodeInput");
    code->setText("131676");
    QTimer::singleShot(100, []() {
        if (auto dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) dialog->reject();
    });
    window.findChild<QPushButton*>("generateBarcode")->click();
    check(!window.findChild<QLabel*>("barcodePreview")->pixmap().isNull(), "barcode displayed before save choice");
    capture(window, "generator");
    code->setText("195028");
    check(window.findChild<QLabel*>("barcodePreview")->pixmap().isNull(), "edited number clears stale barcode");
    ScanDialog scanner;
    scanner.show(); capture(scanner, "camera"); scanner.close();
    window.close();
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
