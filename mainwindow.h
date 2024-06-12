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
#include <QTimer>
#include <QSettings>
#include <QMouseEvent>
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
    bool initializing;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionQuit_triggered();

    void on_actionSave_JSON_triggered();

    void on_actionGitHub_triggered();

    void on_actionRescan_Refresh_triggered();

    void on_actionAbout_triggered();

    void on_actionIgnore_C4_Reallocation_Event_Count_toggled(bool enabled);

    void on_actionHEX_toggled(bool enabled);

    void on_actionUse_Fahrenheit_toggled(bool enabled);

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
    QSpacerItem *buttonStretch;
    QSettings settings;

    QMenu *menuDisk;
    QMenu *toolMenu;

    QJsonArray devices;
    QStringList deviceOutputs;
    QJsonObject globalObj;
    QString globalHealth;
    bool globalIsNvme;
    QVector<QPair<QString, int>> globalNvmeSmartOrdered;

    void onNextButtonClicked();
    void onPrevButtonClicked();
    void updateNavigationButtons(int currentIndex);
    void scanDevices();
    void updateUI();
    void selfTestHandler(const QString &mode, const QString &name, const QString &minutes);
    void populateWindow(const QJsonObject &tempObj, const QString &health, const QVector<QPair<QString, int>>& nvmeLogOrdered = QVector<QPair<QString, int>>());
    void addNvmeLogTable(const QVector<QPair<QString, int>>& nvmeLogOrdered);
    void addSmartAttributesTable(const QJsonArray &attributes);
    QString getSmartctlOutput(const QStringList &arguments, bool root);
    QString toTitleCase(const QString& sentence);
    void clearButtonGroup();
    QString initiateSelfTest(const QString &testType, const QString &deviceNode);
    void cancelSelfTest(const QString &deviceNode);
    void mousePressEvent(QMouseEvent*);
};
#endif
