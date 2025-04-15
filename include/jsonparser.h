#pragma once

#include <QString>
#include <QVector>
#include <QPair>

class JsonParser
{
public:
    JsonParser() = default;

    QVector<QPair<QString, int>> parse(const QString &json);

private:
    QString removeQuotes(const QString &s);
};
