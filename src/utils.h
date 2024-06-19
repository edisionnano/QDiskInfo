#ifndef UTILS_H
#define UTILS_H

#include <QMainWindow>
#include <QProcess>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QButtonGroup>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QSettings>
#include <QMouseEvent>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPushButton>
#include <QAbstractButton>
#include <QMenu>
#include <QApplication>

class utils
{
public:
    utils();

    void clearButtonGroup(QButtonGroup* buttonGroup, QHBoxLayout* horizontalLayout, QSpacerItem* buttonStretch, QMenu* menuDisk);
    QString getSmartctlPath(bool initializing);
    QString getSmartctlOutput(const QStringList &arguments, bool root, bool initializing);
    QPair<QStringList, QJsonArray> scanDevices(bool initializing);
    QString initiateSelfTest(const QString &testType, const QString &deviceNode);
    void cancelSelfTest(const QString &deviceNode);
    void selfTestHandler(const QString &mode, const QString &name, const QString &minutes);
    QString toTitleCase(const QString& sentence);
};

#endif // UTILS_H
