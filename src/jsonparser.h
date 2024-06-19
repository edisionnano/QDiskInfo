#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QString>
#include <QVector>
#include <QStringList>
#include <QPair>

class JsonParser
{
public:
    JsonParser();

    QVector<QPair<QString, int>> parse(const QString &json);

private:
    QString removeQuotes(const QString &s);
};

#endif
