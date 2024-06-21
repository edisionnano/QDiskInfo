#include "jsonparser.h"

JsonParser::JsonParser()
{
}

QVector<QPair<QString, int>> JsonParser::parse(const QString &json)
{
    QVector<QPair<QString, int>> data;
    auto lines = json.split('\n');
    bool found = false;
    bool skip = false;

    for (const auto &line : lines) {
        auto trimmedLine = line.trimmed();
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
            }
            auto colon_pos = trimmedLine.indexOf(":");
            if (colon_pos != -1) {
                auto key = removeQuotes(trimmedLine.left(colon_pos));
                auto valueString = trimmedLine.mid(colon_pos + 1).trimmed();
                valueString.chop(1);
                auto value = valueString.toInt();
                data.append(qMakePair(key, value));
            }
        }
    }
    return data;
}

QString JsonParser::removeQuotes(const QString &s)
{
    auto result = s.trimmed();
    if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
        return result.mid(1, result.size() - 2);
    }
    return result;
}
