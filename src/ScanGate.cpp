#include "ScanGate.h"
void ScanGate::reset() {
    last.clear();
    candidate.clear();
    matches = 0;
    blanks = 0;
}
QString ScanGate::observe(const QString& code) {
    if (code.isEmpty()) {
        candidate.clear();
        matches = 0;
        if (blanks < 3)
            ++blanks;
        if (blanks == 3)
            last.clear();
        return {};
    }
    blanks = 0;
    if (code == last) {
        candidate.clear();
        matches = 0;
        return {};
    }
    if (candidate != code) {
        candidate = code;
        matches = 1;
        return {};
    }
    if (++matches < 2)
        return {};
    last = code;
    candidate.clear();
    matches = 0;
    return code;
}
