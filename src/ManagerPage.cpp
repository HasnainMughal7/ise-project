#include "MainWindow.h"
#include "Widgets.h"
#include "Barcode.h"
#include "ImageReader.h"
#include "Numbers.h"
#include "Reports.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

static bool acceptForm(QDialog& dialog, QFormLayout& form) {
    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    form.addRow(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    return dialog.exec() == QDialog::Accepted;
}
QWidget* MainWindow::inventoryPage() {
    auto* p = new QWidget;
    auto* v = new QVBoxLayout(p);
    inventory = table(
        {"ID", "Barcode", "Category", "Product", "Unit", "Price", "Tax", "Discount", "Free stock"});
    inventory->setObjectName("inventory");
    v->addWidget(inventory);
    auto* r = new QHBoxLayout;
    v->addLayout(r);
    auto* add = button("Add product + barcode", "addProduct");
    auto* edit = button("Edit product");
    auto* stock = button("Update stock");
    auto* barcode = button("Show / save label");
    auto* manual = button("Generate label by number", "manualLabel");
    addWidgets(r, {add, edit, stock, barcode, manual});
    r->addStretch();
    connect(add, &QPushButton::clicked, this, [this] {
        productForm(false);
    });
    connect(edit, &QPushButton::clicked, this, [this] {
        productForm(true);
    });
    connect(manual, &QPushButton::clicked, this, [this] {
        bool ok = false;
        QString n = QInputDialog::getText(this, "Generate a label", "Six digits starting with 1–9",
                                          QLineEdit::Normal, {}, &ok);
        if (!ok)
            return;
        QString error = validateNumber(n);
        if (!error.isEmpty()) {
            say(error, true);
            return;
        }
        label(n, true);
    });
    connect(stock, &QPushButton::clicked, this, &MainWindow::setStock);
    connect(barcode, &QPushButton::clicked, this, [this] {
        label(selected(inventory, 1));
    });
    return p;
}
void MainWindow::productForm(bool edit) {
    if (!access.isManager())
        return;
    Product p;
    if (edit) {
        auto* found = sales.findCode(selected(inventory, 1));
        if (!found) {
            say("Select a product.", true);
            return;
        }
        p = *found;
    }
    if (sales.data().categories.isEmpty()) {
        say("Add a category in Categories & seasons first.", true);
        return;
    }
    QDialog d(this);
    d.setWindowTitle(edit ? "Edit product" : "New product");
    QFormLayout f(&d);
    QLineEdit name(p.name), acro(p.acronym);
    name.setObjectName("productName");
    acro.setObjectName("productAcronym");
    acro.setMaxLength(8);
    QComboBox category, unit;
    category.setObjectName("productCategory");
    for (const auto& c : sales.data().categories)
        category.addItem(c.id + "  " + c.name, c.id);
    if (edit)
        category.setCurrentIndex(category.findData(p.category));
    unit.addItems({"count", "weight"});
    if (edit)
        unit.setCurrentText(p.unit);
    auto* price = decimal(2, 1000000, p.price / 100.0);
    auto* tax = decimal(2, 1000000, p.tax / 100.0);
    auto* discount = decimal(2, 1000000, p.discount / 100.0);
    auto* stock = decimal(3, 1000000, p.available / 1000.0);
    f.addRow("Category", &category);
    f.addRow("Product name", &name);
    f.addRow("Acronym", &acro);
    f.addRow("Unit", &unit);
    f.addRow("Unit price (PKR)", price);
    f.addRow("Tax per unit (PKR)", tax);
    f.addRow("Discount per unit (PKR)", discount);
    f.addRow("Free stock", stock);
    while (acceptForm(d, f)) {
        p.name = name.text();
        p.acronym = acro.text();
        p.category = category.currentData().toString();
        p.unit = unit.currentText();
        p.price = qRound64(price->value() * 100);
        p.tax = qRound64(tax->value() * 100);
        p.discount = qRound64(discount->value() * 100);
        p.available = qRound64(stock->value() * 1000);
        QString code, e;
        if (!sales.putProduct(p, code, e)) {
            QMessageBox::warning(&d, "Product", e);
            continue;
        }
        refresh();
        say("Product saved. Barcode " + code + ".");
        label(code);
        return;
    }
}
void MainWindow::setStock() {
    const Product* p = sales.findCode(selected(inventory, 1));
    if (!p) {
        say("Select a product.", true);
        return;
    }
    bool ok = false;
    double n = QInputDialog::getDouble(this, "Update free stock", p->name, p->available / 1000.0, 0,
                                       1000000, p->unit == "count" ? 0 : 3, &ok);
    if (ok) {
        QString e;
        if (!sales.setStock(p->id, qRound64(n * 1000), e)) {
            say(e, true);
            return;
        }
        refresh();
        say("Inventory updated.");
    }
}
void MainWindow::label(const QString& code, bool standalone) {
    if (standalone) {
        bool configured = false;
        for (const Category& category : sales.data().categories) {
            if (category.id == code.left(2)) {
                configured = true;
                break;
            }
        }
        if (!configured) {
            say("Configure category " + code.left(2) + " before generating this label.", true);
            return;
        }
    }
    const Product* p = sales.findCode(code);
    if (!p && !standalone) {
        say("Select a product to view its label.", true);
        return;
    }
    QDialog d(this);
    d.setWindowTitle((p ? p->name : "Code 39 label") + " | " + code);
    QVBoxLayout v(&d);
    QImage image = makeBarcode(code);
    QLabel picture;
    picture.setPixmap(QPixmap::fromImage(image).scaledToWidth(420, Qt::FastTransformation));
    v.addWidget(&picture);
    QLabel info(p ? "Saved in data/" + p->imagePath : "Use Save PNG as… to export this label.");
    v.addWidget(&info);
    QDialogButtonBox b(QDialogButtonBox::Close);
    auto* save = b.addButton("Save PNG as…", QDialogButtonBox::ActionRole);
    v.addWidget(&b);
    connect(&b, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    connect(save, &QPushButton::clicked, &d, [&] {
        QString path =
            QFileDialog::getSaveFileName(&d, "Save label", "barcode_" + code + ".png", "PNG (*.png)");
        if (path.isEmpty())
            return;
        if (!path.endsWith(".png", Qt::CaseInsensitive))
            path += ".png";
        QString e;
        if (!saveBarcode(image, path, e))
            QMessageBox::warning(&d, "Save label", e);
    });
    d.exec();
}
QWidget* MainWindow::settingsPage() {
    auto* p = new QWidget;
    auto* v = new QVBoxLayout(p);
    v->addWidget(new QLabel("Product categories"));
    categoryTable = table({"Prefix", "Category", "Acronym"});
    v->addWidget(categoryTable);
    auto* a = button("Add category", "addCategory");
    v->addWidget(a, 0, Qt::AlignLeft);
    v->addWidget(new QLabel("Seasonal discounts"));
    seasonTable = table({"Start", "End", "Rate", "Active"});
    seasonTable->setObjectName("seasons");
    v->addWidget(seasonTable);
    auto* r = new QHBoxLayout;
    v->addLayout(r);
    auto* add = button("Add season", "addSeason");
    auto* toggle = button("Toggle active", "toggleSeason");
    addWidgets(r, {add, toggle});
    r->addStretch();
    connect(a, &QPushButton::clicked, this, &MainWindow::categoryForm);
    connect(add, &QPushButton::clicked, this, &MainWindow::seasonForm);
    connect(toggle, &QPushButton::clicked, this, [this] {
        int i = seasonTable->currentRow();
        if (i < 0) {
            say("Select a season.", true);
            return;
        }
        Season s = sales.data().seasons[i];
        s.active = !s.active;
        QString e;
        if (!sales.putSeason(i, s, e)) {
            say(e, true);
            return;
        }
        refresh();
        say("Season updated.");
    });
    return p;
}
void MainWindow::categoryForm() {
    QDialog d(this);
    d.setWindowTitle("New category");
    QFormLayout f(&d);
    QLineEdit id, name, acro;
    id.setObjectName("categoryId");
    name.setObjectName("categoryName");
    acro.setObjectName("categoryShort");
    id.setMaxLength(2);
    acro.setMaxLength(8);
    f.addRow("Prefix (10–99)", &id);
    f.addRow("Category name", &name);
    f.addRow("Acronym", &acro);
    while (acceptForm(d, f)) {
        QString e;
        if (!sales.addCategory({id.text(), name.text(), acro.text()}, e)) {
            QMessageBox::warning(&d, "Category", e);
            continue;
        }
        refresh();
        say("Category saved.");
        break;
    }
}
void MainWindow::seasonForm() {
    QDialog d(this);
    d.setWindowTitle("New season");
    QFormLayout f(&d);
    auto* start = calendar(QDate::currentDate());
    auto* end = calendar(QDate::currentDate().addDays(7));
    auto* rate = decimal(2, 100, 10);
    QCheckBox active("Active");
    active.setChecked(true);
    f.addRow("Start", start);
    f.addRow("End", end);
    f.addRow("Discount %", rate);
    f.addRow(&active);
    while (acceptForm(d, f)) {
        QString e;
        if (!sales.putSeason(
                -1, {start->date(), end->date(), qRound(rate->value() * 100), active.isChecked()}, e)) {
            QMessageBox::warning(&d, "Season", e);
            continue;
        }
        refresh();
        say("Season saved.");
        break;
    }
}
QWidget* MainWindow::reportsPage() {
    auto* p = new QWidget;
    auto* v = new QVBoxLayout(p);
    auto* r = new QHBoxLayout;
    v->addLayout(r);
    reportKind = new QComboBox;
    reportKind->setObjectName("reportKind");
    reportKind->addItems({"Incomplete orders", "Completed orders", "Cash sales",
                          "All sales & discounts", "Top 10 sold", "Top 10 remaining"});
    period = new QComboBox;
    period->addItems({"Today", "This week", "This month", "This year", "Custom"});
    from = calendar(QDate::currentDate());
    through = calendar(QDate::currentDate());
    addWidgets(r, {reportKind, period, from, through});
    reportTable = table({});
    reportTable->setObjectName("reportTable");
    v->addWidget(reportTable);
    reportSummary = new QLabel;
    v->addWidget(reportSummary);
    connect(reportKind, &QComboBox::currentIndexChanged, this, [this] {
        buildReport();
    });
    connect(period, &QComboBox::currentIndexChanged, this, [this] {
        buildReport();
    });
    connect(from, &QDateEdit::dateChanged, this, [this] {
        buildReport();
    });
    connect(through, &QDateEdit::dateChanged, this, [this] {
        buildReport();
    });
    return p;
}
void MainWindow::refreshManager() {
    inventory->setRowCount(0);
    categoryTable->setRowCount(0);
    seasonTable->setRowCount(0);
    for (const auto& p : sales.data().products)
        addRow(inventory, {p.id, p.barcode, p.category, p.name, p.unit, fixedText(p.price, 2),
                           fixedText(p.tax, 2), fixedText(p.discount, 2), fixedText(p.available, 3)});
    for (const auto& c : sales.data().categories)
        addRow(categoryTable, {c.id, c.name, c.acronym});
    for (const auto& s : sales.data().seasons)
        addRow(seasonTable, {s.start.toString(Qt::ISODate), s.end.toString(Qt::ISODate),
                             fixedText(s.rate, 2) + "%", s.active ? "Yes" : "No"});
    buildReport();
}
void MainWindow::buildReport() {
    auto kind = static_cast<ReportKind>(reportKind->currentIndex());
    auto range = static_cast<Period>(period->currentIndex());
    bool cash = kind == ReportKind::Cash;
    period->setVisible(cash);
    from->setVisible(cash);
    through->setVisible(cash);
    from->setEnabled(range == Period::Custom);
    through->setEnabled(range == Period::Custom);
    if (range != Period::Custom) {
        auto dates = periodRange(range, QDate::currentDate());
        QSignalBlocker a(from), b(through);
        from->setDate(dates.first);
        through->setDate(dates.second);
    }
    Report rep = makeReport(sales.data(), kind, from->date(), through->date());
    reportTable->setRowCount(0);
    reportTable->setColumnCount(rep.columns.size());
    reportTable->setHorizontalHeaderLabels(rep.columns);
    if (cash && from->date() > through->date()) {
        reportSummary->setText("Choose an end date on or after the start.");
        return;
    }
    for (const auto& row : rep.rows)
        addRow(reportTable, row);
    reportSummary->setText(rep.summary);
}
