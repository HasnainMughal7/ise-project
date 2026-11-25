#include <QLineEdit>
#include "MainWindow.h"
#include "TextStore.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CornerStore");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"data-dir", "Folder with store text files", "directory"});
    parser.process(app);
    QDir bin(QCoreApplication::applicationDirPath());
    QString directory = parser.value("data-dir");
    if (directory.isEmpty()) {
        directory = bin.filePath("data");
        if (!QFileInfo::exists(directory + "/Product.txt"))
            directory = bin.filePath("../data");
    }
    directory = QDir(directory).absolutePath();
    QLockFile lock(QDir(directory).filePath("Store.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) {
        QMessageBox::critical(nullptr, "Cannot open store",
                              "Data folder is in use, missing or read-only.\n" + directory);
        return 1;
    }
    QString error;
    Access access(directory);
    TextStore files(directory);
    Checkout sales(files, access);
    if (!sales.load(error)) {
        QMessageBox::critical(nullptr, "Store file error", error + "\n" + directory);
        return 1;
    }
    while (!access.configured()) {
        bool ok = false;
        QString pin = QInputDialog::getText(nullptr, "First setup", "Manager: choose a 4–12 digit PIN",
                                            QLineEdit::Password, {}, &ok);
        if (!ok)
            return 0;
        QString again = QInputDialog::getText(nullptr, "Confirm setup", "Re-enter the manager PIN",
                                              QLineEdit::Password, {}, &ok);

        if (!ok)
            return 0;
        if (pin != again) {
            QMessageBox::warning(nullptr, "PIN setup", "PINs did not match.");
            continue;
        }
        if (!access.setup(pin, error))
            QMessageBox::warning(nullptr, "PIN setup", error);
    }
    MainWindow window(sales, access, directory);
    window.show();
    return app.exec();
}
