#include "jsonparser.h"

#include <QStringList>

QVector<QPair<QString, int>> JsonParser::parse(const QString &json)
{
    QVector<QPair<QString, int>> data;
    QStringList lines = json.split('\n');
    bool found = false;
    bool skip = false;

    for (const QString &line : std::as_const(lines)) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.contains("[")) {
            skip = true;
            continue;
        }
        if (trimmedLine.contains("]")) {
            skip = false;
            continue;
        }
        if (skip) {
            continue;
        }
        if (trimmedLine.contains("nvme_smart_health_information_log")) {
            found = true;
            continue;
        }
        if (found) {
            if (trimmedLine.contains("}")) {
                break;
            } else if (trimmedLine.contains("nsid")) { // smartctl now adds an nsid field, skip it for now
                continue;
            }
            qsizetype colonPos = trimmedLine.indexOf(":");
            if (colonPos != -1) {
                QString key = removeQuotes(trimmedLine.left(colonPos));
                QString valueString = trimmedLine.mid(colonPos + 1).trimmed();
                valueString.chop(1);
                int value = valueString.toInt();
                data.append(qMakePair(key, value));
            }
        }
    }
    return data;
}

QString JsonParser::removeQuotes(const QString &s)
{
    QString result = s.trimmed();
    if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
        return result.mid(1, result.size() - 2);
    }
    return result;
}
