#include "MainWindow.h"
#include "Barcode.h"
#include "ImageReader.h"
#include "Numbers.h"
#include "Receipt.h"
#include "ScanDialog.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>

static QPushButton* button(const QString& text, QBoxLayout* layout, const QString& name = "")
{
    auto b = new QPushButton(text);
    b->setObjectName(name);
    layout->addWidget(b);
    return b;
}
static QTableWidget* table(const QStringList& headers)
{
    auto t = new QTableWidget(0, headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->setAlternatingRowColors(true);
    return t;
}
static void addRow(QTableWidget* t, const QStringList& cells)
{
    int row = t->rowCount();
    t->insertRow(row);
    for (int i = 0; i < cells.size(); ++i)
        t->setItem(row, i, new QTableWidgetItem(cells[i]));
}

MainWindow::MainWindow(Checkout& service, const QString& path) : checkout(service), directory(path)
{
    setWindowTitle("Corner Store - PoS V1");
    resize(1180, 810);
    setMinimumSize(1020, 720);
    auto central = new QWidget;
    auto layout = new QVBoxLayout(central);
    layout->setContentsMargins(22, 16, 22, 16);
    auto heading = new QLabel("Corner Store");
    heading->setStyleSheet("font-size:28px;font-weight:700;color:#153449;");
    layout->addWidget(heading);
    layout->addWidget(new QLabel("POINT OF SALE  /  INCREMENT 2"));
    auto tabs = new QTabWidget;
    tabs->setObjectName("tabs");
    tabs->addTab(makeCheckoutPage(), "New order");
    tabs->addTab(makeCatalogPage(), "Products");
    tabs->addTab(makeOrdersPage(), "Saved orders");
    tabs->addTab(makeBarcodePage(), "Barcode generator");
    layout->addWidget(tabs, 1);
    message = new QLabel("Ready. Scan a product to start an order.");
    message->setWordWrap(true);
    message->setTextFormat(Qt::PlainText);
    layout->addWidget(message);
    setCentralWidget(central);
    setStyleSheet("QMainWindow,QDialog{background:#f4f7f9;} QWidget{font-family:'Segoe UI';font-size:14px;color:#203a4c;}"
        "QTabWidget::pane{border:1px solid #dbe4e9;background:white;border-radius:8px;}"
        "QTabBar::tab{padding:10px 24px;background:#e8eef2;} QTabBar::tab:selected{background:white;color:#087d73;font-weight:bold;}"
        "QPushButton{background:#e6eff3;border:1px solid #cfdee5;border-radius:6px;padding:8px 12px;}"
        "QPushButton:hover{background:#d6e8ee;} QPushButton:disabled{color:#8898a2;}"
        "QPushButton#scanButton,QPushButton#issueButton{background:#087d73;color:white;font-weight:bold;border:0;}"
        "QLineEdit,QComboBox,QDoubleSpinBox{padding:5px;border:1px solid #c9d7de;border-radius:5px;background:white;}"
        "QTableWidget{border:1px solid #dbe4e9;background:white;alternate-background-color:#f6f9fb;gridline-color:#e4ebef;}"
        "QHeaderView::section{background:#eaf0f4;padding:7px;border:0;font-weight:bold;}"
        "QGroupBox{font-weight:bold;border:1px solid #dbe4e9;border-radius:8px;margin-top:12px;padding:10px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 5px;} QLabel#totals{font-size:17px;font-weight:bold;color:#087d73;}");
    refresh();
}

QWidget* MainWindow::makeCheckoutPage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(16, 14, 16, 14);
    auto top = new QHBoxLayout;
    auto scanButton = button("Scan product with camera", top, "scanButton");
    top->addWidget(new QLabel("Quantity for next scan"));
    quantity = new QDoubleSpinBox;
    quantity->setObjectName("quantity");
    quantity->setDecimals(3);
    quantity->setRange(0.001, 1000000);
    quantity->setValue(1);
    top->addWidget(quantity);
    top->addStretch();
    layout->addLayout(top);
    layout->addWidget(new QLabel("Count uses whole items. Weight uses kilograms. Repeated scans combine quantities."));
    lastProduct = new QLabel("Scan a label to see its product specifications here.");
    lastProduct->setObjectName("lastProduct");
    lastProduct->setWordWrap(true);
    lastProduct->setTextFormat(Qt::PlainText);
    layout->addWidget(lastProduct);
    cartTable = table({"Product", "Unit price PKR", "Quantity", "Unit", "Line payable PKR"});
    cartTable->setObjectName("cartTable");
    layout->addWidget(cartTable, 1);
    auto actions = new QHBoxLayout;
    auto description = button("Description", actions);
    auto change = button("Apply quantity to row", actions);
    auto remove = button("Remove row", actions);
    auto clear = button("Clear order", actions);
    layout->addLayout(actions);
    auto bottom = new QHBoxLayout;
    auto customerBox = new QGroupBox("Customer");
    auto cf = new QFormLayout(customerBox);
    customer = new QLineEdit;
    customer->setObjectName("customer");
    customer->setPlaceholderText("Customer name");
    customer->setMaxLength(100);
    contact = new QLineEdit;
    contact->setPlaceholderText("Optional");
    contact->setMaxLength(100);
    cf->addRow("Name", customer);
    cf->addRow("Contact", contact);
    bottom->addWidget(customerBox);
    auto payBox = new QGroupBox("Payment record");
    auto pf = new QFormLayout(payBox);
    status = new QComboBox;
    status->addItems({"paid", "on-hold", "in-progress"});
    status->setObjectName("orderStatus");
    mode = new QComboBox;
    mode->addItems({"cash", "card", "wallet"});
    merchant = new QLineEdit;
    merchant->setPlaceholderText("Bank or wallet name");
    merchant->setMaxLength(100);
    payment = new QLineEdit;
    payment->setObjectName("payment");
    payment->setMaxLength(18);
    pf->addRow("Status", status);
    pf->addRow("Mode", mode);
    pf->addRow("Merchant", merchant);
    pf->addRow("Paid PKR", payment);
    bottom->addWidget(payBox);
    layout->addLayout(bottom);
    totalsLabel = new QLabel;
    totalsLabel->setObjectName("totals");
    totalsLabel->setWordWrap(true);
    layout->addWidget(totalsLabel);
    auto issue = new QPushButton("Generate bill and save order");
    issue->setObjectName("issueButton");
    layout->addWidget(issue);
    connect(scanButton, &QPushButton::clicked, this, [this]() { scan(true); });
    connect(description, &QPushButton::clicked, this, [this]() {
        int row = cartTable->currentRow();
        describe(row < 0 ? nullptr : checkout.product(checkout.items()[row].productId));
    });
    connect(change, &QPushButton::clicked, this, [this]() {
        QString error;
        qint64 value = 0;
        parseFixed(QString::number(quantity->value(), 'f', 3), 3, value);
        if (!checkout.changeQuantity(cartTable->currentRow(), value, error)) showError(error);
        else refresh();
    });
    connect(remove, &QPushButton::clicked, this, [this]() {
        checkout.remove(cartTable->currentRow());
        refresh();
    });
    connect(clear, &QPushButton::clicked, this, [this]() {
        if (checkout.items().isEmpty() || QMessageBox::question(this, "Clear order", "Discard this unsaved order?") == QMessageBox::Yes) {
            checkout.clear();
            lastProduct->setText("Scan a label to see its product specifications here.");
            refresh();
        }
    });
    connect(status, &QComboBox::currentIndexChanged, this, [this]() { updatePayment(); });
    connect(mode, &QComboBox::currentIndexChanged, this, [this]() { updatePayment(); });
    connect(issue, &QPushButton::clicked, this, &MainWindow::generateBill);
    return page;
}

QWidget* MainWindow::makeCatalogPage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Product specifications loaded from Product.txt"));
    catalogTable = table({"ID", "Barcode", "Product", "Unit", "Price PKR", "Available"});
    layout->addWidget(catalogTable, 1);
    auto actions = new QHBoxLayout;
    auto scanButton = button("Scan to view description", actions);
    auto details = button("Selected product details", actions);
    auto reload = button("Reload files", actions);
    auto folder = button("Open data folder", actions);
    layout->addLayout(actions);
    connect(scanButton, &QPushButton::clicked, this, [this]() { scan(false); });
    connect(details, &QPushButton::clicked, this, [this]() {
        int row = catalogTable->currentRow();
        describe(row < 0 ? nullptr : &checkout.data().products[row]);
    });
    connect(reload, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!checkout.load(error)) showError(error);
        else { refresh(); message->setText("Files reloaded."); }
    });
    connect(folder, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
    });
    return page;
}

QWidget* MainWindow::makeOrdersPage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Saved bills keep their original prices. Stock is reserved once when a bill is generated."));
    ordersTable = table({"Order ID", "Date", "Customer", "Status", "Payable PKR"});
    ordersTable->setObjectName("ordersTable");
    layout->addWidget(ordersTable, 1);
    auto actions = new QHBoxLayout;
    auto view = button("View bill", actions);
    auto pay = button("Mark paid", actions);
    auto archiveButton = button("Archive paid order", actions);
    layout->addLayout(actions);
    connect(view, &QPushButton::clicked, this, [this]() {
        int row = ordersTable->currentRow();
        if (row >= 0) showBill(ordersTable->item(row, 0)->text());
    });
    connect(pay, &QPushButton::clicked, this, [this]() { changeStatus(true); });
    connect(archiveButton, &QPushButton::clicked, this, [this]() { changeStatus(false); });
    return page;
}

QWidget* MainWindow::makeBarcodePage()
{
    auto page = new QWidget;
    auto layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Generate a Code 39 label using our manual encoder. Enter 6 digits, starting with 1 to 9."));
    auto row = new QHBoxLayout;
    auto input = new QLineEdit;
    input->setObjectName("barcodeInput");
    input->setMaxLength(32);
    input->setPlaceholderText("123456");
    row->addWidget(input);
    auto generate = button("Generate and save PNG", row, "generateBarcode");
    layout->addLayout(row);
    auto preview = new QLabel("Your barcode appears here");
    preview->setObjectName("barcodePreview");
    preview->setAlignment(Qt::AlignCenter);
    preview->setMinimumHeight(260);
    layout->addWidget(preview, 1);
    auto saved = new QLabel;
    saved->setWordWrap(true);
    saved->setTextFormat(Qt::PlainText);
    layout->addWidget(saved);
    layout->addWidget(new QLabel("Creating a label does not add a product record. Use a matching barcode in Product.txt."));
    connect(input, &QLineEdit::textChanged, this, [preview, saved]() {
        preview->clear(); saved->clear();
    });
    connect(generate, &QPushButton::clicked, this, [this, input, preview, saved]() {
        QString error = validateNumber(input->text());
        if (!error.isEmpty()) { showError(error); return; }
        QImage image = makeBarcode(input->text());
        preview->setPixmap(QPixmap::fromImage(image));
        QString path = QFileDialog::getSaveFileName(this, "Save barcode", QDir(directory).filePath("barcodes/barcode_" + input->text() + ".png"), "PNG image (*.png)");
        if (path.isEmpty()) return;
        if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
        if (!saveBarcode(image, path, error)) showError(error);
        else saved->setText("Saved: " + path);
    });
    return page;
}

void MainWindow::refresh()
{
    cartTable->setRowCount(0);
    for (const OrderLine& line : checkout.items()) {
        const Product* p = checkout.product(line.productId);
        qint64 net = lineAmount(p->price, line.quantity) + lineAmount(p->tax, line.quantity) - lineAmount(p->discount, line.quantity);
        addRow(cartTable, {p->name, fixedText(p->price, 2), fixedText(line.quantity, 3), p->unit, fixedText(net, 2)});
    }
    catalogTable->setRowCount(0);
    for (const Product& p : checkout.data().products)
        addRow(catalogTable, {p.id, p.barcode, p.name, p.unit, fixedText(p.price, 2), fixedText(p.available, 3)});
    ordersTable->setRowCount(0);
    for (int i = checkout.data().orders.size() - 1; i >= 0; --i) {
        const Order& o = checkout.data().orders[i];
        for (const Bill& b : checkout.data().bills)
            if (b.orderId == o.id) addRow(ordersTable, {o.id, o.date, o.customer, o.status, fixedText(b.totals.payable(), 2)});
    }
    Totals total = checkout.totals();
    totalsLabel->setText("Subtotal " + fixedText(total.subtotal, 2) + "   + Tax " + fixedText(total.tax, 2) +
        "   - Discount " + fixedText(total.discount, 2) + "     Total PKR " + fixedText(total.payable(), 2));
    updatePayment();
    findChild<QPushButton*>("issueButton")->setEnabled(!checkout.items().isEmpty() && !checkout.isBlocked());
    findChild<QPushButton*>("scanButton")->setEnabled(!checkout.isBlocked());
}

void MainWindow::updatePayment()
{
    bool paid = status->currentText() == "paid";
    payment->setEnabled(paid);
    payment->setText(fixedText(paid ? checkout.totals().payable() : 0, 2));
    merchant->setEnabled(mode->currentText() != "cash");
}

void MainWindow::scan(bool addToOrder)
{
    ScanDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    const Product* p = checkout.findBarcode(dialog.barcode());
    if (!p) {
        lastProduct->setText("No product matches the scanned barcode.");
        showError("Unknown barcode " + dialog.barcode() + ". Add its product to Product.txt, then reload.");
        return;
    }
    lastProduct->setText(productDescription(*p).replace('\n', "   |   "));
    if (!addToOrder) { describe(p); return; }
    QString name = p->name, error;
    qint64 value = 0;
    parseFixed(QString::number(quantity->value(), 'f', 3), 3, value);
    if (!checkout.add(dialog.barcode(), value, error)) { showError(error); return; }
    refresh();
    message->setText("Added " + name + ". Select its row and press Description for all specifications.");
}

void MainWindow::describe(const Product* p)
{
    if (!p) { showError("Select a product row first."); return; }
    QMessageBox box(this);
    box.setWindowTitle("Product specifications");
    box.setTextFormat(Qt::PlainText);
    box.setText(productDescription(*p));
    box.exec();
}

void MainWindow::generateBill()
{
    Order order;
    order.customer = customer->text(); order.contact = contact->text(); order.status = status->currentText();
    order.mode = mode->currentText(); order.merchant = merchant->text();
    if (!parseFixed(payment->text(), 2, order.payment)) {
        showError("Enter a nonnegative payment with at most 2 decimal places.");
        return;
    }
    Bill bill;
    QString error;
    if (!checkout.issueBill(order, QDateTime::currentDateTime(), bill, error)) { showError(error); refresh(); return; }
    customer->clear(); contact->clear(); merchant->clear(); quantity->setValue(1);
    lastProduct->setText("Scan a label to start the next order.");
    refresh();
    message->setText("Order " + bill.orderId + " saved. Bill " + bill.id + " is ready.");
    showBill(bill.orderId);
}

void MainWindow::showBill(const QString& orderId)
{
    for (const Order& order : checkout.data().orders) {
        if (order.id != orderId) continue;
        for (const Bill& bill : checkout.data().bills) {
            if (bill.orderId != orderId) continue;
            QDialog dialog(this);
            dialog.setWindowTitle("Bill " + bill.id);
            dialog.resize(820, 660);
            auto layout = new QVBoxLayout(&dialog);
            auto receipt = new QTextBrowser;
            receipt->setHtml(receiptHtml(bill, order));
            layout->addWidget(receipt);
            auto close = new QPushButton("Close");
            layout->addWidget(close);
            connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
            dialog.exec();
            return;
        }
    }
}

void MainWindow::changeStatus(bool paid)
{
    int row = ordersTable->currentRow();
    if (row < 0) { showError("Select a saved order."); return; }
    QString id = ordersTable->item(row, 0)->text();
    QString prompt = paid ? "Confirm the full payment has been received using the recorded payment mode?" : "Archive this paid order?";
    if (QMessageBox::question(this, "Update order", prompt) != QMessageBox::Yes) return;
    QString error;
    bool ok = paid ? checkout.markPaid(id, error) : checkout.archive(id, error);
    if (!ok) showError(error);
    refresh();
}

void MainWindow::showError(const QString& error)
{
    QMessageBox box(QMessageBox::Warning, "Please check", "", QMessageBox::Ok, this);
    box.setTextFormat(Qt::PlainText);
    box.setText(error);
    box.exec();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!checkout.items().isEmpty() && QMessageBox::question(this, "Unsaved order", "Close and discard the unsaved order?") != QMessageBox::Yes)
        event->ignore();
    else event->accept();
}
