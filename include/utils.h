#pragma once

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QMenu>
#include <QSpacerItem>

class QProcess;

class utils
{
public:
    utils() = default;

    static void startPrivilegedProcess(QProcess &process, const QStringList &arguments);

    QString clearButtonGroup(QButtonGroup* buttonGroup, QHBoxLayout* horizontalLayout, QSpacerItem* buttonStretch, QMenu* menuDisk);
    QString getSmartctlPath();
    QString getSmartctlOutput(const QStringList &arguments, bool root, bool initializing);
    QPair<QStringList, QJsonArray> scanDevices(bool initializing);
    QString initiateSelfTest(const QString &testType, const QString &deviceNode);
    void cancelSelfTest(const QString &deviceNode);
    void selfTestHandler(const QString &mode, const QString &name, const QString &minutes);
    QString toTitleCase(const QString& sentence);
};
