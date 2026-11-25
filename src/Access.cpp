#include "Access.h"
#include <QCryptographicHash>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QRegularExpression>

static QByteArray digest(const QString& pin, const QByteArray& salt) {
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, pin.toUtf8(), salt, 100000,
                                              32);
}
Access::Access(const QString& directory) : file(QDir(directory).filePath("Manager.key")) {}
bool Access::configured() const {
    return QFile::exists(file);
}
bool Access::setup(const QString& pin, QString& error) {
    if (configured() || !QRegularExpression("\\A[0-9]{4,12}\\z").match(pin).hasMatch()) {
        error = "Create a 4 to 12 digit PIN during first setup.";
        return false;
    }
    QByteArray salt(16, '\0');
    for (char& c : salt)
        c = char(QRandomGenerator::system()->bounded(256));
    QByteArray bytes = salt.toHex() + '\n' + digest(pin, salt).toHex() + '\n';
    QSaveFile out(file);
    if (!out.open(QIODevice::WriteOnly) || out.write(bytes) != bytes.size() || !out.commit()) {
        error = "Cannot save the manager PIN in this data folder.";
        return false;
    }
    return true;
}
bool Access::accepts(const QString& pin) const {
    QFile in(file);
    if (!in.open(QIODevice::ReadOnly) || in.size() > 160 || pin.size() < 4 || pin.size() > 12)
        return false;
    QByteArray salt = QByteArray::fromHex(in.readLine().trimmed());
    QByteArray expected = QByteArray::fromHex(in.readLine().trimmed());
    return salt.size() == 16 && expected.size() == 32 && digest(pin, salt) == expected;
}
bool Access::unlock(const QString& pin) {
    manager = accepts(pin);
    return manager;
}
