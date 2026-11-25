#pragma once
#include "Checkout.h"
#include <QMainWindow>
class QCheckBox;
class CameraScanner;
class QTabWidget;
class QTableWidget;
class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QLabel;
class QDateEdit;
class QCloseEvent;
class MainWindow : public QMainWindow {
  public:
    MainWindow(Checkout& sales, Access& access, const QString& folder);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    Checkout& sales;
    Access& access;
    QString folder;
    QTabWidget* tabs;
    CameraScanner* camera;
    bool cameraOn = false;
    QLabel *roleLabel, *cartTitle, *total, *notice, *preview, *reportSummary;
    QPushButton *roleButton, *cameraButton;
    QTableWidget *cartTable, *history, *inventory, *categoryTable, *seasonTable, *reportTable;
    QLineEdit *code, *customer, *contact, *merchant, *tender;
    QDoubleSpinBox *scanQty, *special;
    QComboBox *held, *devices, *mode, *reportKind, *period;
    QDateEdit *from, *through;
    QString lastCode;
    QCheckBox* lookup;
    QWidget* cashierPage();
    QWidget* ordersPage();
    QWidget* inventoryPage();
    QWidget* settingsPage();
    QWidget* reportsPage();
    void refresh();
    void refreshCart();
    void refreshManager();
    void buildReport();
    void fillForm();
    void say(QString text, bool error = false);
    void setRole();
    void openSaved(const QString& id);
    void scan(const QString& value);
    void showProduct(const Product& product);
    void changeQty();
    void description();
    void importImage();
    void finish(const QString& status);
    bool discardCurrent();
    void viewBill(const QString& id);
    void productForm(bool edit);
    void setStock();
    void categoryForm();
    void seasonForm();
    void label(const QString& barcode, bool standalone = false);
};
