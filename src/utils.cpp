#include "utils.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

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

QString utils::getSmartctlPath() {
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
        command = getSmartctlPath();
    }

    if (!getSmartctlPath().isEmpty()) {
        process.start(command, arguments);
        process.waitForFinished(-1);
    }

    if (process.exitCode() == 126 || process.exitCode() == 127) {
        QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("QDiskInfo needs root access in order to read S.M.A.R.T. data!"));
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
    QString smartctlPath = getSmartctlPath();
    QStringList commandList;
    QStringList deviceOutputs;

    for (const QJsonValue &value : std::as_const(devices)) {
        QJsonObject device = value.toObject();
        QString deviceName = device["name"].toString();
        commandList.append((smartctlPath + " --all --json=o %1").arg(deviceName));
    }
    QString command = commandList.join(" ; ");

    if (smartctlPath.isEmpty()) {
        QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("smartctl was not found, please install it!"));
        QTimer::singleShot(0, qApp, &QApplication::quit);
    }

    QString allDevicesOutput = getSmartctlOutput({"sh", "-c", command}, true, initializing);

    qsizetype startIndex = 0;
    qsizetype endIndex = 0;

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

QString utils::initiateSelfTest(const QString &testType, const QString &deviceNode)
{
    QProcess process;
    QString command = getSmartctlPath();
    QStringList arguments;
    arguments << command << "--json=o" << "-t" << testType << deviceNode;

    process.start("pkexec", arguments);
    process.waitForFinished(-1);

    if (process.isOpen()) {
        return process.readAllStandardOutput();
    } else {
        return QString();
    }
}

void utils::cancelSelfTest(const QString &deviceNode)
{
    QProcess process;
    QString command = getSmartctlPath();
    QStringList arguments;
    arguments << command << "-X" << deviceNode;

    process.start("pkexec", arguments);
    process.waitForFinished(-1);

    if (process.exitCode() == 126 || process.exitCode() == 127) {
        QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("QDiskInfo needs root access in order to abort a self-test!"));
    } else if (process.exitCode() == QProcess::NormalExit) {
        QMessageBox::information(nullptr, QObject::tr("Test Requested"), QObject::tr("The self-test has been aborted"));
    } else {
        QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("Error: Something went wrong"));
    }
}

void utils::selfTestHandler(const QString &mode, const QString &name, const QString &minutes) {
    QString output = initiateSelfTest(mode, name);
    if (output.isEmpty()) {
        QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("QDiskInfo needs root access in order to request a self-test!"));
    } else {
        QJsonDocument testDoc = QJsonDocument::fromJson(output.toUtf8());
        QJsonObject testObj = testDoc.object();
        QJsonObject smartctlObj = testObj.value("smartctl").toObject();
        int exitStatus = smartctlObj.value("exit_status").toInt();

        QJsonArray outputArray = smartctlObj["output"].toArray();
        static const QRegularExpression regex("\\((\\d+%)\\s*(\\w+)\\)");

        QString percentage;
        for (const QJsonValue &value : outputArray) {
            QString line = value.toString();
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                percentage = match.captured(0);
                break;
            }
        }
        percentage.replace("remaining", QObject::tr("remaining"));
        percentage.replace("completed", QObject::tr("completed"));

        if (exitStatus == 4) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(QObject::tr("Test Already Running"));
            msgBox.setText(QObject::tr("A self-test is already being performed") + " " + percentage + "\n" + QObject::tr("You can press the Ok button in order to abort the test that is currently running"));
            msgBox.setIcon(QMessageBox::Warning);

            msgBox.addButton(QMessageBox::Cancel);
            QPushButton *abortButton = msgBox.addButton(QMessageBox::Ok);

            msgBox.exec();

            if (msgBox.clickedButton() == abortButton) {
                cancelSelfTest(name);
            }
        } else if (exitStatus == 0) {
            QString infoMessage = QObject::tr("A self-test has been requested successfully");
            if (minutes != "0") {
                infoMessage = infoMessage + "\n" + QObject::tr("It will be completed after") + " " + minutes + " " + QObject::tr("minutes");
            }
            QMessageBox::information(nullptr, QObject::tr("Test Requested"), infoMessage);
        } else {
            QMessageBox::critical(nullptr, QObject::tr("QDiskInfo Error"), QObject::tr("Error: Something went wrong"));
        }
    }
}

QString utils::toTitleCase(const QString& sentence) {
    QString result;
    bool capitalizeNext = true;

    for (const QChar& c : sentence) {
        if (c.isLetter()) {
            if (capitalizeNext) {
                result += c.toUpper();
                capitalizeNext = false;
            } else {
                result += c.toLower();
            }
        } else {
            result += c;
            if (c == ' ') {
                capitalizeNext = true;
            }
        }
    }

    return result;
}
