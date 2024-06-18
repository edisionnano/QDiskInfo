#include "utils.h"

utils::utils() {}

void utils::clearButtonGroup(QButtonGroup* buttonGroup, QHBoxLayout* horizontalLayout, QSpacerItem* buttonStretch, QMenu* menuDisk)
{
    QList<QAbstractButton*> buttons = buttonGroup->buttons();
    for (QAbstractButton* button : buttons) {
        buttonGroup->removeButton(button);
        delete button;
    }
    horizontalLayout->removeItem(buttonStretch);
    delete buttonStretch;
    menuDisk->clear();
}

QString utils::getSmartctlPath(bool initializing) {
    QStringList paths = QString::fromLocal8Bit(qgetenv("PATH")).split(QDir::listSeparator(), Qt::SkipEmptyParts);

    paths << "/usr/sbin" << "/usr/local/sbin";

    for (const QString &path : paths) {
        QString absolutePath = QDir(path).absoluteFilePath("smartctl");
        if (QFile::exists(absolutePath) && QFileInfo(absolutePath).isExecutable()) {
            return absolutePath;
        }
    }

    return QString();
}

QString utils::getSmartctlOutput(const QStringList &arguments, bool root, bool initializing)
{
    QProcess process;
    QString command;

    if (root) {
        command = "pkexec";
    } else {
        command = getSmartctlPath(initializing);
    }

    if (!getSmartctlPath(initializing).isEmpty()) {
        process.start(command, arguments);
        process.waitForFinished(-1);
    }

    if (process.exitCode() == 127) {
        QMessageBox::critical(nullptr, QObject::tr("KDiskInfo Error"), QObject::tr("KDiskInfo needs root access in order to read S.M.A.R.T. data!"));
        if (initializing) {
            QTimer::singleShot(0, qApp, &QApplication::quit);
        }
        return QString();
    }

    if (process.isOpen()) {
        return process.readAllStandardOutput();
    } else {
        return QString();
    }
}


QPair<QStringList, QJsonArray> utils::scanDevices(bool initializing)
{
    QString output = getSmartctlOutput({"--scan", "--json"}, false, initializing);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObj = doc.object();
    QJsonArray devices = jsonObj["devices"].toArray();
    QString smartctlPath = getSmartctlPath(initializing);
    QStringList commandList;
    QStringList deviceOutputs;

    for (const QJsonValue &value : std::as_const(devices)) {
        QJsonObject device = value.toObject();
        QString deviceName = device["name"].toString();
        commandList.append(QString(smartctlPath + " --all --json=o %1").arg(deviceName));
    }
    QString command = commandList.join(" ; ");

    if (smartctlPath.isEmpty()) {
        QMessageBox::critical(nullptr, QObject::tr("KDiskInfo Error"), QObject::tr("smartctl was not found, please install it!"));
        QTimer::singleShot(0, qApp, &QApplication::quit);
    }

    QString allDevicesOutput = getSmartctlOutput({"sh", "-c", command}, true, initializing);

    int startIndex = 0;
    int endIndex = 0;

    static const QRegularExpression regex("\\}\\n\\{");

    while ((endIndex = allDevicesOutput.indexOf(regex, startIndex)) != -1) {
        ++endIndex;
        QString jsonFragment = allDevicesOutput.mid(startIndex, endIndex - startIndex);
        deviceOutputs.append(jsonFragment);
        startIndex = endIndex;
    }

    if (startIndex < allDevicesOutput.size()) {
        QString jsonFragment = allDevicesOutput.mid(startIndex);
        deviceOutputs.append(jsonFragment);
    }

    if (!allDevicesOutput.isEmpty()) {
        return QPair<QStringList, QJsonArray>(deviceOutputs, devices);
    } else {
        return QPair<QStringList, QJsonArray>(QStringList(), QJsonArray());
    }
}
