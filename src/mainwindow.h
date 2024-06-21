#pragma once

#include <QAction>
#include <QActionGroup>
#include <QJsonObject>
#include <QMainWindow>
#include <QMouseEvent>
#include <QSettings>
#include <QTableWidget>
#include <QWidget>
#include <cmath>

#include "statusdot.h"
#include "custombutton.h"
#include "jsonparser.h"
#include "utils.h"
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

    void on_actionCyclic_Navigation_toggled(bool arg1);

    void on_actionUse_GB_instead_of_TB_toggled(bool arg1);

private:
    Ui::MainWindow *ui;
    QLocale locale;
    utils Utils;
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
    QAction *actionCyclic_Navigation;

    QMenu *menuDevice;
    QMenu *menuDisk;
    QMenu *selfTestMenu;
    QAction *selfTestLogAction;
    QActionGroup *disksGroup;

    QJsonArray devices;
    QStringList deviceOutputs;
    QJsonObject globalObj;
    QString globalHealth;
    bool globalIsNvme;
    QVector<QPair<QString, int>> globalNvmeSmartOrdered;

    void onNextButtonClicked();
    void onPrevButtonClicked();
    void updateNavigationButtons(int currentIndex);
    void updateUI();
    void populateWindow(const QJsonObject &tempObj, const QString &health, const QVector<QPair<QString, int>>& nvmeLogOrdered = QVector<QPair<QString, int>>());
    void addNvmeLogTable(const QVector<QPair<QString, int>>& nvmeLogOrdered);
    void addSmartAttributesTable(const QJsonArray &attributes);
    void mousePressEvent(QMouseEvent*);
};
