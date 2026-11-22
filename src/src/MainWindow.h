#pragma once
#include "Checkout.h"
#include <QMainWindow>

class QTableWidget;
class QLabel;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class MainWindow : public QMainWindow {
public:
    MainWindow(Checkout& checkout, const QString& dataDirectory);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    Checkout& checkout;
    QString directory;
    QTableWidget *cartTable, *catalogTable, *ordersTable;
    QLabel *totalsLabel, *message, *lastProduct;
    QLineEdit *customer, *contact, *merchant, *payment;
    QComboBox *status, *mode;
    QDoubleSpinBox *quantity;
    QWidget* makeCheckoutPage();
    QWidget* makeCatalogPage();
    QWidget* makeOrdersPage();
    QWidget* makeBarcodePage();
    void refresh();
    void updatePayment();
    void scan(bool addToOrder);
    void describe(const Product* product);
    void generateBill();
    void showBill(const QString& orderId);
    void changeStatus(bool paid);
    void showError(const QString& error);
};
