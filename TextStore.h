#pragma once
#include "Store.h"
#include <QMap>
#include <QStringList>

class TextStore : public Store {
public:
    explicit TextStore(const QString& directory);
    bool load(StoreState& state, QString& error) override;
    bool save(const StoreState& state, QString& error) override;
    bool recover(QString& error);
private:
    QString directory;
    bool readRows(const QString& name, QVector<QStringList>& rows, QString& error);
    bool writeFile(const QString& name, const QByteArray& bytes, QString& error);
    QMap<QString, QByteArray> serialize(const StoreState& state) const;
};
