#pragma once
#include <QString>
// Return a code once per physical presentation of the label.
class ScanGate {
  public:
    QString observe(const QString& code);
    void reset();

  private:
    QString last, candidate;
    int matches = 0, blanks = 0;
};
