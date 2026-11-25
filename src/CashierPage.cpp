#include "MainWindow.h"
#include "Widgets.h"
#include "CameraScanner.h"
#include "ImageReader.h"
#include "Numbers.h"
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMediaDevices>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>

QWidget* MainWindow::cashierPage() {
    auto* p = new QWidget;
    auto* v = new QVBoxLayout(p);
    auto* titleRow = new QHBoxLayout;
    cartTitle = new QLabel;
    cartTitle->setObjectName("cartTitle");
    titleRow->addWidget(cartTitle);
    titleRow->addStretch();
    held = new QComboBox;
    held->setObjectName("heldOrders");
    held->setMinimumWidth(145);
    auto* resume = button("Resume", "resume");
    addWidgets(titleRow, {new QLabel("Held orders"), held, resume});
    v->addLayout(titleRow);
    connect(resume, &QPushButton::clicked, this, [this] {
        openSaved(held->currentData().toString());
    });
    auto* scanRow = new QHBoxLayout;
    v->addLayout(scanRow);
    code = new QLineEdit;
    code->setObjectName("barcode");
    code->setPlaceholderText("Scan or type six digits, then Enter");
    code->setMaxLength(6);
    scanQty = decimal(3, 1000000, 1);
    scanQty->setObjectName("scanQty");
    scanQty->setMinimum(0.001);
    auto* add = button("Add", "scanAdd");
    auto* image = button("Read PNG");
    addWidgets(scanRow, {code, new QLabel("Scan qty"), scanQty, add, image});
    connect(add, &QPushButton::clicked, this, [this] {
        scan(code->text());
    });
    connect(code, &QLineEdit::returnPressed, this, [this] {
        scan(code->text());
    });
    connect(image, &QPushButton::clicked, this, &MainWindow::importImage);
    auto* cameraRow = new QHBoxLayout;
    v->addLayout(cameraRow);
    devices = new QComboBox;
    for (const auto& d : QMediaDevices::videoInputs())
        devices->addItem(d.description());
    if (!devices->count())
        devices->addItem("No camera available");
    cameraButton = button("Start camera", "startCamera");
    addWidgets(cameraRow, {cameraButton, devices});
    lookup = new QCheckBox("Product lookup only");
    lookup->setObjectName("lookupOnly");
    cameraRow->addWidget(lookup);
    cameraRow->addStretch();
    preview = new QLabel;
    preview->setFixedSize(170, 80);
    preview->hide();
    cameraRow->addWidget(preview);
    connect(cameraButton, &QPushButton::clicked, this, [this] {
        if (cameraOn)
            camera->stop();
        else
            camera->start(devices->currentIndex());
    });
    connect(camera, &CameraScanner::frameReady, this, [this](const QImage& img) {
        preview->setPixmap(QPixmap::fromImage(img).scaled(preview->size(), Qt::KeepAspectRatio,
                                                          Qt::FastTransformation));
    });
    connect(camera, &CameraScanner::runningChanged, this, [this](bool on) {
        cameraOn = on;
        preview->setVisible(on);
        devices->setEnabled(!on);
        cameraButton->setText(on ? "Stop camera" : "Start camera");
    });
    connect(camera, &CameraScanner::codeFound, this, [this](const QString& number) {
        scan(number);
    });
    connect(camera, &CameraScanner::problem, this, [this](const QString& e) {
        say(e, true);
    });
    cartTable = table({"Product", "Barcode", "Unit", "Quantity", "Price PKR", "Net line PKR"});
    cartTable->setObjectName("cart");
    cartTable->setMinimumHeight(100);
    v->addWidget(cartTable, 1);
    auto* edits = new QHBoxLayout;
    v->addLayout(edits);
    auto* change = button("Change quantity", "changeQty");
    auto* remove = button("Remove item", "removeItem");
    auto* desc = button("Description");
    auto* clear = button("Discard edits");
    addWidgets(edits, {change, remove, desc, clear});
    edits->addStretch();
    connect(change, &QPushButton::clicked, this, &MainWindow::changeQty);
    connect(cartTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        changeQty();
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (!sales.remove(cartTable->currentRow())) {
            say("Select a line to remove.", true);
            return;
        }
        refreshCart();
        say("Item removed from this cart.");
    });
    connect(desc, &QPushButton::clicked, this, &MainWindow::description);
    connect(clear, &QPushButton::clicked, this, [this] {
        discardCurrent();
    });
    auto* form = new QGridLayout;
    v->addLayout(form);
    customer = new QLineEdit;
    customer->setObjectName("customer");
    customer->setPlaceholderText("Walk-in");
    contact = new QLineEdit;
    contact->setPlaceholderText("Optional");
    mode = new QComboBox;
    mode->addItems({"cash", "card", "wallet"});
    merchant = new QLineEdit;
    merchant->setPlaceholderText("Bank or wallet");
    special = decimal(2, 8);
    special->setSuffix(" %");
    special->setObjectName("specialRate");
    tender = new QLineEdit;
    tender->setObjectName("received");
    tender->setPlaceholderText("Blank = exact payment");
    auto field = [form](int r, int c, QString name, QWidget* w) {
        form->addWidget(new QLabel(name), r, c);
        form->addWidget(w, r, c + 1);
    };
    field(0, 0, "Customer", customer);
    field(0, 2, "Contact", contact);
    field(0, 4, "Special", special);
    field(1, 0, "Payment", mode);
    field(1, 2, "Merchant", merchant);
    field(1, 4, "Received PKR", tender);
    connect(mode, &QComboBox::currentTextChanged, this, [this](QString m) {
        merchant->setEnabled(m != "cash");
    });
    connect(special, &QDoubleSpinBox::valueChanged, this, [this] {
        refreshCart();
    });
    auto* bar = new QHBoxLayout;
    v->addLayout(bar);
    total = new QLabel;
    total->setObjectName("total");
    bar->addWidget(total);
    bar->addStretch();
    auto* hold = button("Hold  ·  F2", "hold");
    hold->setShortcut(QKeySequence("F2"));
    auto* pay = button("Pay / save bill  ·  F4", "pay");
    pay->setShortcut(QKeySequence("F4"));
    addWidgets(bar, {hold, pay});
    connect(hold, &QPushButton::clicked, this, [this] {
        finish("on-hold");
    });
    connect(pay, &QPushButton::clicked, this, [this] {
        finish("paid");
    });
    return p;
}
void MainWindow::fillForm() {
    const Order& o = sales.cart();
    customer->setText(o.customer == "Walk-in" ? "" : o.customer);
    contact->setText(o.contact == "-" ? "" : o.contact);
    mode->setCurrentText(o.mode);
    merchant->setEnabled(o.mode != "cash");
    merchant->setText(o.merchant == "-" ? "" : o.merchant);
    special->setValue(o.specialRate / 100.0);
    tender->clear();
    code->clear();
    scanQty->setValue(1);
    lastCode.clear();
}
void MainWindow::refreshCart() {
    const Order& o = sales.cart();
    int row = cartTable->currentRow();
    cartTable->setRowCount(0);
    for (const Line& l : o.lines)
        addRow(cartTable,
               {l.acronym, l.barcode, l.unit, fixedText(l.quantity, 3), fixedText(l.price, 2),
                fixedText(lineAmount(l.price, l.quantity) + lineAmount(l.tax, l.quantity) -
                              lineAmount(l.discount, l.quantity),
                          2)});
    if (!o.lines.isEmpty())
        cartTable->selectRow(qBound(0, row, int(o.lines.size() - 1)));
    cartTitle->setText(o.id.isEmpty() ? "New order" : "Editing " + o.id + "  ·  " + o.status);
    qint64 due =
        calculate(sales.priced(qRound(special->value() * 100), QDateTime::currentDateTime())).payable();
    total->setText("Payable   PKR " + fixedText(due, 2));
}
void MainWindow::scan(const QString& value) {
    QString e;
    QString number = value.trimmed();
    if (lookup->isChecked()) {
        const Product* p = sales.findCode(number);
        if (!p) {
            say("Unknown barcode.", true);
            return;
        }
        camera->stop();
        lastCode = number;
        code->clear();
        showProduct(*p);
        return;
    }
    if (!sales.add(number, qRound64(scanQty->value() * 1000), e)) {
        say(e, true);
        code->selectAll();
        return;
    }
    lastCode = number;
    code->clear();
    scanQty->setValue(1);
    refreshCart();
    const Product& product = *sales.findCode(number);
    for (int i = 0; i < sales.cart().lines.size(); ++i)
        if (sales.cart().lines[i].productId == product.id)
            cartTable->selectRow(i);
    code->setFocus();
    say(QString("Added %1  |  PKR %2 per %3  |  Tax PKR %4  |  Discount PKR %5  |  Free %6")
            .arg(product.name, fixedText(product.price, 2), product.unit, fixedText(product.tax, 2),
                 fixedText(product.discount, 2), fixedText(product.available, 3)));
}
void MainWindow::changeQty() {
    int i = cartTable->currentRow();
    if (i < 0) {
        say("Select a cart line.", true);
        return;
    }
    Line l = sales.cart().lines[i];
    bool ok = false;
    int digits = l.unit == "count" ? 0 : 3;
    double value =
        QInputDialog::getDouble(this, "Change quantity", "New total quantity for " + l.acronym,
                                l.quantity / 1000.0, digits ? 0.001 : 1, 1000000, digits, &ok);
    if (!ok)
        return;
    QString e;
    if (!sales.quantity(i, qRound64(value * 1000), e)) {
        say(e, true);
        return;
    }
    refreshCart();
    say("Quantity updated.");
}
void MainWindow::description() {
    QString id;
    int i = cartTable->currentRow();
    if (i >= 0)
        id = sales.cart().lines[i].productId;
    for (const Product& p : sales.data().products)
        if ((!id.isEmpty() && p.id == id) || (id.isEmpty() && p.barcode == lastCode)) {
            showProduct(p);
            return;
        }
    say("Select a scanned product.", true);
}
void MainWindow::showProduct(const Product& p) {
    QString details = QString("%1 | %2 | Category %3 | Product ID %4\nUnit: %5    Free stock: %6\n"
                              "Price: PKR %7    Tax: %8    Discount: %9")
                          .arg(p.acronym, p.barcode, p.category, p.id, p.unit, fixedText(p.available, 3),
                               fixedText(p.price, 2), fixedText(p.tax, 2), fixedText(p.discount, 2));
    QMessageBox::information(this, p.name, details + "\nLabel: " + p.imagePath);
}
void MainWindow::importImage() {
    QString path = QFileDialog::getOpenFileName(this, "Read barcode image", {},
                                                "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty())
        return;
    QString e;
    auto img = loadImage(path, e);
    if (img.isNull()) {
        say(e, true);
        return;
    }
    auto result = readBarcode(img);
    if (result.ok())
        scan(result.number);
    else
        say(result.error, true);
}
void MainWindow::finish(const QString& status) {
    int rate = qRound(special->value() * 100);
    QDateTime now = QDateTime::currentDateTime();
    qint64 received = 0;
    if (status == "paid") {
        received = calculate(sales.priced(rate, now)).payable();
        if (!tender->text().trimmed().isEmpty() && !parseFixed(tender->text().trimmed(), 2, received)) {
            say("Enter received money with at most two decimals.", true);
            return;
        }
    }
    QString id, e;
    if (!sales.finish(status, customer->text(), contact->text(), mode->currentText(), merchant->text(),
                      rate, received, now, id, e)) {
        say(e, true);
        return;
    }
    camera->stop();
    fillForm();
    refresh();
    held->setCurrentIndex(held->findData(id));
    if (held->currentIndex() < 0 && held->count())
        held->setCurrentIndex(0);
    say(status == "paid" ? "Paid order " + id : "Order " + id + " held. Continue using Resume.");
    code->setFocus();
    if (status == "paid")
        viewBill(id);
}
