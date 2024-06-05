#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include <cmath>

#include "statusdot.h"
#include "custombutton.h"
#include "jsonparser.h"
#include "./ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionExit_triggered();

    void on_actionSave_JSON_triggered();

    void on_actionGitHub_triggered();

private:
    Ui::MainWindow *ui;
    QButtonGroup *buttonGroup;
    QHBoxLayout *horizontalLayout;
    QLabel *diskName, *temperatureValue, *healthStatusValue;
    QLineEdit *firmwareLineEdit, *serialNumberLineEdit, *typeLineEdit, *protocolLineEdit, *deviceNodeLineEdit;
    QLineEdit *totalReadsLineEdit, *totalWritesLineEdit, *rotationRateLineEdit, *powerOnCountLineEdit, *powerOnHoursLineEdit;
    QTableWidget *tableWidget;
    QPushButton *prevButton, *nextButton;
    QColor goodColor, cautionColor, badColor, naColor;

    QJsonObject deviceJson;

    void onNextButtonClicked();
    void onPrevButtonClicked();
    void updateNavigationButtons(int currentIndex);
    void scanDevices();
    void populateWindow(const QJsonObject &tempObj, const QString &health, const QVector<QPair<QString, int>>& nvmeLogOrdered = QVector<QPair<QString, int>>());
    void addNvmeLogTable(const QVector<QPair<QString, int>>& nvmeLogOrdered);
    void addSmartAttributesTable(const QJsonArray &attributes);
    QString getSmartctlOutput(const QStringList &arguments, bool root);
    QString toTitleCase(const QString& sentence);
};
#endif
