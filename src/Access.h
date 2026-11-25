#pragma once
#include <QString>
class Access {
  public:
    explicit Access(const QString& directory);
    bool configured() const;
    bool setup(const QString& pin, QString& error);
    bool accepts(const QString& pin) const;
    bool unlock(const QString& pin);
    void lock() {
        manager = false;
    }
    bool isManager() const {
        return manager;
    }

  private:
    QString file;
    bool manager = false;
};
