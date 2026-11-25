#include <QComboBox>
#include <QLineEdit>
#include "MainWindow.h"
#include "Widgets.h"
#include "CameraScanner.h"
#include "Receipt.h"
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QScrollArea>

MainWindow::MainWindow(Checkout& service, Access& auth, const QString& directory)
    : sales(service), access(auth), folder(directory) {
    setWindowTitle("CornerStore | PoS v2");
    resize(1100, 760);
    auto* root = new QWidget;
    auto* layout = new QVBoxLayout(root);
    auto* top = new QHBoxLayout;
    auto* title = new QLabel("CornerStore");
    title->setObjectName("brand");
    roleLabel = new QLabel;
    roleButton = button("Manager sign-in", "roleButton");
    top->addWidget(title);
    top->addStretch();
    addWidgets(top, {roleLabel, roleButton});
    layout->addLayout(top);
    camera = new CameraScanner(this);
    tabs = new QTabWidget;
    tabs->setObjectName("workspace");
    auto addPage = [this](QWidget* p, QString name) {
        auto* scroll = new QScrollArea;
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidgetResizable(true);
        scroll->setWidget(p);
        tabs->addTab(scroll, name);
    };
    addPage(cashierPage(), "Checkout");
    addPage(ordersPage(), "Orders && bills");
    addPage(inventoryPage(), "Inventory");
    addPage(settingsPage(), "Categories && seasons");
    addPage(reportsPage(), "Reports");
    layout->addWidget(tabs);
    notice = new QLabel("Ready to scan.");
    notice->setWordWrap(true);
    notice->setObjectName("notice");
    layout->addWidget(notice);
    setCentralWidget(root);
    setStyleSheet(
        "QMainWindow,QScrollArea{background:#f2f5f2;} QWidget{font-family:'Segoe UI','DejaVu "
        "Sans';font-size:13px;color:#162e29;}"
        "QLabel#brand{font-size:27px;font-weight:bold;color:#124e43;} "
        "QLabel#total,QLabel#cartTitle{font-size:18px;font-weight:bold;}"
        "QPushButton{background:#e4ede8;border:1px solid #a2b5ac;border-radius:5px;padding:5px 10px;}"
        "QPushButton:hover{background:#d1e7da;} "
        "QPushButton#pay,QPushButton#hold{background:#146652;color:white;font-weight:bold;}"
        "QLineEdit,QComboBox,QDateEdit,QDoubleSpinBox{background:white;padding:3px;border:1px solid "
        "#9eb2a9;border-radius:4px;}"
        "QTableWidget{background:white;alternate-background-color:#f1f6f3;gridline-color:#e2e9e4;}"
        "QHeaderView::section{background:#deebe4;padding:4px;border:0;} QTabBar::tab{padding:7px 12px;}"
        "QTabBar::tab:selected{background:#cee6da;color:#0d4f40;}");
    connect(roleButton, &QPushButton::clicked, this, [this] {
        if (access.isManager())
            access.lock();
        else {
            bool ok = false;
            QString pin = QInputDialog::getText(this, "Manager sign-in", "Manager PIN",
                                                QLineEdit::Password, {}, &ok);
            if (!ok)
                return;
            if (!access.unlock(pin)) {
                say("Incorrect manager PIN.", true);
                return;
            }
        }
        setRole();
        refresh();
        say(access.isManager() ? "Manager workspace unlocked." : "Cashier workspace active.");
    });
    connect(tabs, &QTabWidget::currentChanged, this, [this](int i) {
        if (i != 0 && cameraOn)
            camera->stop();
    });
    setRole();
    fillForm();
    refresh();
}
void MainWindow::setRole() {
    bool manager = access.isManager();
    roleLabel->setText(manager ? "Manager" : "Cashier");
    roleButton->setText(manager ? "Lock manager workspace" : "Manager sign-in");
    if (!manager && tabs->currentIndex() > 1)
        tabs->setCurrentIndex(0);
    for (int i = 2; i < tabs->count(); ++i)
        tabs->setTabVisible(i, manager);
}
void MainWindow::say(QString text, bool error) {
    notice->setStyleSheet(error ? "color:#a33228;font-weight:bold;" : "color:#20533d;");
    notice->setText(text);
}
void MainWindow::refresh() {
    QString oldHeld = held->currentData().toString();
    held->clear();
    history->setRowCount(0);
    const auto& orders = sales.data().orders;
    for (auto it = orders.crbegin(); it != orders.crend(); ++it) {
        const Order& o = *it;
        if (!settled(o))
            held->addItem(o.id + "   " + o.customer, o.id);
        addRow(history, {o.id, o.billId, o.customer, o.status, o.opened.toString("yyyy-MM-dd"),
                         QString::number(calculate(o).payable() / 100.0, 'f', 2)});
    }
    int i = held->findData(oldHeld);
    if (i >= 0)
        held->setCurrentIndex(i);
    refreshCart();
    if (access.isManager())
        refreshManager();
}
bool MainWindow::discardCurrent() {
    if ((!sales.cart().lines.isEmpty() || !sales.cart().id.isEmpty()) &&
        QMessageBox::question(
            this, "Leave cart?",
            "Discard unsaved cart edits? A previously saved order keeps its last saved version.") !=
            QMessageBox::Yes)
        return false;
    sales.discard();
    fillForm();
    refresh();
    return true;
}
void MainWindow::closeEvent(QCloseEvent* e) {
    if (discardCurrent()) {
        camera->stop();
        e->accept();
    } else
        e->ignore();
}
void MainWindow::openSaved(const QString& id) {
    const Order* o = sales.findOrder(id);
    if (!o) {
        say("Select a saved order.", true);
        return;
    }
    if (o->status == "archived") {
        say("Archived bill is available in View / PDF.", true);
        return;
    }
    if (sales.cart().id == id) {
        tabs->setCurrentIndex(0);
        return;
    }
    QString pin;
    if (o->status == "paid") {
        bool ok = false;
        pin = QInputDialog::getText(this, "Approve this paid order", "Manager PIN for order " + id,
                                    QLineEdit::Password, {}, &ok);

        if (!ok)
            return;
    }
    if (!discardCurrent())
        return;
    QString err;
    if (!sales.openOrder(id, pin, QDateTime::currentDateTime(), err)) {
        say(err, true);
        return;
    }
    tabs->setCurrentIndex(0);
    fillForm();
    refresh();
    code->setFocus();
    say("Editing saved order " + id + ".");
}
QWidget* MainWindow::ordersPage() {
    auto* p = new QWidget;
    auto* v = new QVBoxLayout(p);
    history = table({"Order", "Bill", "Customer", "Status", "Opened", "Payable PKR"});
    history->setObjectName("history");
    v->addWidget(history);
    auto* bar = new QHBoxLayout;
    v->addLayout(bar);
    auto* open = button("Open for editing", "openOrder");
    auto* view = button("View bill / PDF", "viewBill");
    auto* cancel = button("Cancel unpaid order");
    auto* archive = button("Archive paid order");
    addWidgets(bar, {open, view, cancel, archive});
    bar->addStretch();
    connect(open, &QPushButton::clicked, this, [this] {
        openSaved(selected(history));
    });
    connect(history, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        openSaved(selected(history));
    });
    connect(view, &QPushButton::clicked, this, [this] {
        viewBill(selected(history));
    });
    connect(cancel, &QPushButton::clicked, this, [this] {
        QString id = selected(history), e;
        if (id.isEmpty())
            return;
        if (QMessageBox::question(this, "Cancel order", "Cancel " + id + " and release its stock?") !=
            QMessageBox::Yes)
            return;
        bool active = sales.cart().id == id;
        if (!sales.cancelHeld(id, e)) {
            say(e, true);
            return;
        }
        if (active)
            fillForm();
        refresh();
        say("Order cancelled. Reserved stock returned.");
    });
    connect(archive, &QPushButton::clicked, this, [this] {
        QString e;
        if (QMessageBox::question(this, "Archive paid bill",
                                  "Archive this order permanently as read-only?") != QMessageBox::Yes)
            return;
        if (!sales.archive(selected(history), e)) {
            say(e, true);
            return;
        }
        refresh();
        say("Paid bill archived.");
    });
    return p;
}
void MainWindow::viewBill(const QString& id) {
    const Order* order = sales.findOrder(id);
    if (!order) {
        say("Select a saved bill.", true);
        return;
    }
    Order copy = *order;
    QDialog dialog(this);
    dialog.setWindowTitle("Bill " + copy.billId);
    dialog.resize(680, 570);
    QVBoxLayout layout(&dialog);
    auto* receipt = new QTextBrowser;
    receipt->setHtml(receiptHtml(copy));
    layout.addWidget(receipt);
    QDialogButtonBox buttons(QDialogButtonBox::Close);
    auto* pdf = buttons.addButton("Save PDF", QDialogButtonBox::ActionRole);
    layout.addWidget(&buttons);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(pdf, &QPushButton::clicked, &dialog, [&, copy] {
        QString path = QFileDialog::getSaveFileName(&dialog, "Save bill PDF",
                                                    "Bill-" + copy.billId + ".pdf", "PDF (*.pdf)");
        if (path.isEmpty())
            return;
        if (!path.endsWith(".pdf", Qt::CaseInsensitive))
            path += ".pdf";
        QString e;
        if (!exportPdf(copy, path, e))
            QMessageBox::warning(&dialog, "PDF", e);
        else
            say("PDF saved to " + path);
    });
    dialog.exec();
}
