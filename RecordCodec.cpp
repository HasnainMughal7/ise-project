#include "RecordCodec.h"

bool splitRecord(const QString& line, QStringList& fields)
{
    fields.clear();
    int i = 0;
    while (i < line.size()) {
        while (i < line.size() && line[i].isSpace()) ++i;
        if (i == line.size()) return false;
        if (line[i] == '#')
            return !fields.isEmpty() && line.mid(i + 1).trimmed().isEmpty();
        QString field;
        if (line[i] == '"') {
            ++i;
            bool closed = false;
            while (i < line.size()) {
                if (line[i] == '"') {
                    ++i;
                    if (i < line.size() && line[i] == '"') {
                        field += '"';
                        ++i;
                    } else {
                        closed = true;
                        break;
                    }
                } else {
                    field += line[i++];
                }
            }
            if (!closed) return false;
        } else {
            while (i < line.size() && !line[i].isSpace() && line[i] != '#') {
                if (line[i] == '"') return false;
                field += line[i++];
            }
        }
        if (field.contains('\n') || field.contains('\r') || field.contains('\t')) return false;
        if (i < line.size() && !line[i].isSpace() && line[i] != '#') return false;
        fields.append(field);
    }
    return false;
}

QString joinRecord(const QStringList& fields)
{
    QStringList encoded;
    for (QString field : fields) {
        if (field.isEmpty() || field.contains(' ') || field.contains('"') || field.contains('#')) {
            field.replace("\"", "\"\"");
            field = '"' + field + '"';
        }
        encoded.append(field);
    }
    return encoded.join(' ') + " #\n";
}
