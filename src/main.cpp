#include "MainWindow.h"
#include "TextStore.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CornerStore");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"data-dir", "Folder containing the four text files", "directory"});
    parser.process(app);
    QDir binary(QCoreApplication::applicationDirPath());
    QString directory = parser.value("data-dir");
    if (directory.isEmpty()) {
        directory = binary.filePath("data");
        if (!QFileInfo::exists(directory + "/Product.txt")) directory = binary.filePath("../data");
    }
    directory = QDir(directory).absolutePath();
    QLockFile lock(QDir(directory).filePath("Store.lock"));
    if (!lock.tryLock(0)) {
        QMessageBox::critical(nullptr, "Cannot open store", "This data folder is already in use, missing or not writable.\n" + directory);
        return 1;
    }
    TextStore store(directory);
    Checkout checkout(store);
    QString error;
    if (!checkout.load(error)) {
        QMessageBox::critical(nullptr, "Store file error", error + "\n" + directory);
        return 1;
    }
    MainWindow window(checkout, directory);
    window.show();
    return app.exec();
}
