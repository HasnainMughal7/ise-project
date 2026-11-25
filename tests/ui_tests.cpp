#include "MainWindow.h"
#include "TextStore.h"
#include "TestKit.h"
#include "CameraScanner.h"
#include <QApplication>
#include <QProcess>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    Checks t;
    if (argc == 2) {
        QTemporaryDir data;
        copySeed(argv[1], data.path());
        for (const QString& phase : QStringList{"hold", "resume"}) {
            QProcess process;
            process.setProcessChannelMode(QProcess::ForwardedChannels);
            process.start(QCoreApplication::applicationFilePath(), {phase, data.path()});
            bool ok = process.waitForStarted() && process.waitForFinished(25000) &&
                      process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
            t.check(ok, "separate GUI process", phase);
            if (!ok)
                break;
        }
        return t.result();
    }
    if (argc != 3)
        return 2;
    QString phase = argv[1], folder = argv[2], e, id;
    Access access(folder);
    TextStore disk(folder);
    auto click = [](MainWindow& w, QString name) {
        auto* b = w.findChild<QPushButton*>(name);
        if (!b)
            qFatal("Missing action");
        b->click();
    };
    auto enter = [](MainWindow& w, QString value) {
        auto* in = w.findChild<QLineEdit*>("barcode");
        in->setText(value);
        QMetaObject::invokeMethod(in, "returnPressed");
    };
    auto screenshot = [&](MainWindow& w, QString name) {
        QString folder = qEnvironmentVariable("ISE_UI_IMAGES");
        if (!folder.isEmpty()) {
            app.processEvents();
            QDir().mkpath(folder);
            if (!w.grab().save(folder + "/" + name + ".png"))
                qFatal("Saving a QA screenshot failed");
        }
    };
    if (phase == "hold") {
        t.check(access.setup("3469", e), "first-time PIN", e);
        Checkout s(disk, access);
        t.check(s.load(e), "load first UI");
        MainWindow w(s, access, folder);
        w.show();
        auto* tabs = w.findChild<QTabWidget*>("workspace");
        t.check(!tabs->isTabVisible(2) && tabs->isTabVisible(1), "cashier tabs");
        auto* camera = w.findChild<CameraScanner*>();
        QMetaObject::invokeMethod(camera, "codeFound", Q_ARG(QString, QString("123456")));
        enter(w, "123456");
        enter(w, "131676");
        t.check(s.cart().lines.size() == 2 && s.cart().lines[0].quantity == 2000 &&
                    w.findChild<QLabel*>("notice")->text().contains("Tax PKR"),
                "scan event and keyboard entry");
        w.findChild<QLineEdit*>("customer")->setText("Demo customer");
        click(w, "hold");
        t.check(s.data().orders.size() == 1 && s.cart().lines.isEmpty(), "single-click hold");
        id = s.data().orders[0].id;
    }
    if (phase == "resume") {
        Access auth(folder);
        Checkout s(disk, auth);
        t.check(s.load(e), "restart with same data");
        if (s.data().orders.size() != 1)
            return 1;
        id = s.data().orders[0].id;
        MainWindow w(s, auth, folder);
        w.show();
        click(w, "resume");
        t.check(s.cart().id == id && s.cart().lines.size() == 2, "one-click resume on fresh window");
        auto* cart = w.findChild<QTableWidget*>("cart");
        cart->selectRow(1);
        click(w, "removeItem");
        enter(w, "123456");
        cart->selectRow(0);
        QTimer::singleShot(0, [] {
            auto* d = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            if (d) {
                d->setDoubleValue(2);
                d->accept();
            }
        });
        click(w, "changeQty");
        w.findChild<QDoubleSpinBox*>("scanQty")->setValue(0.5);
        enter(w, "345678");
        t.check(s.cart().lines.size() == 2 && s.cart().lines[0].quantity == 2000 &&
                    s.cart().lines[1].quantity == 500,
                "remove, add, change and weighted scan using widgets");
        screenshot(w, "resumed-cart");
        bool receiptVisible = false;
        QTimer::singleShot(0, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (dialog) {
                auto* b = dialog->findChild<QTextBrowser*>();
                receiptVisible = b && b->toPlainText().contains("662.50");
                dialog->accept();
            }
        });
        click(w, "pay");
        t.check(receiptVisible && s.findOrder(id)->status == "paid", "pay shows full bill");
        t.check(s.findCode("123456")->available == 22000 && s.findCode("131676")->available == 16000,
                "visible edit persisted as net delta");
        QTimer::singleShot(0, [] {
            auto* d = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            if (d) {
                d->setTextValue("3469");
                d->accept();
            }
        });
        click(w, "roleButton");
        auto* tabs = w.findChild<QTabWidget*>("workspace");
        t.check(tabs->isTabVisible(2) && tabs->isTabVisible(4),
                "manager unlock shows inventory and reports");
        tabs->setCurrentIndex(2);
        screenshot(w, "inventory");
        tabs->setCurrentIndex(4);
        w.findChild<QComboBox*>("reportKind")->setCurrentIndex(3);
        t.check(w.findChild<QTableWidget*>("reportTable")->rowCount() == 1, "manager sales report");
        screenshot(w, "sales");
        tabs->setCurrentIndex(3);
        QTimer::singleShot(0, [] {
            auto* form = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            form->findChild<QLineEdit*>("categoryId")->setText("56");
            form->findChild<QLineEdit*>("categoryName")->setText("Frozen Foods");
            form->findChild<QLineEdit*>("categoryShort")->setText("FROZEN");
            form->accept();
        });
        click(w, "addCategory");
        t.check(s.data().categories.last().id == "56", "add category from manager UI");
        QTimer::singleShot(0, [] {
            qobject_cast<QDialog*>(QApplication::activeModalWidget())->accept();
        });
        click(w, "addSeason");
        t.check(s.data().seasons.size() == 1 && s.data().seasons[0].active,
                "create active season in UI");
        w.findChild<QTableWidget*>("seasons")->selectRow(0);
        click(w, "toggleSeason");
        t.check(!s.data().seasons[0].active, "deactivate season in UI");
        tabs->setCurrentIndex(2);
        QTimer::singleShot(0, [] {
            auto* form = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            form->findChild<QLineEdit*>("productName")->setText("Frozen Peas");
            form->findChild<QLineEdit*>("productAcronym")->setText("PEAS");
            auto* picker = form->findChild<QComboBox*>("productCategory");
            picker->setCurrentIndex(picker->findData("56"));
            QTimer::singleShot(0, [] {
                qobject_cast<QDialog*>(QApplication::activeModalWidget())->accept();
            });
            form->accept();
        });
        click(w, "addProduct");
        t.check(s.findCode("560000") && QFile::exists(folder + "/barcodes/barcode_560000.png"),
                "manager product form generates category-coded PNG");

        QTimer::singleShot(0, [] {
            auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            dialog->setTextValue("770001");
            dialog->accept();
        });
        click(w, "manualLabel");
        t.check(w.findChild<QLabel*>("notice")->text().contains("Configure category 77"),
                "manual label also requires a configured prefix");
        click(w, "roleButton");
        t.check(!tabs->isTabVisible(4) && tabs->currentIndex() == 0, "lock returns to cashier");
    }
    return t.result();
}
