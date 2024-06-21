#include "mainwindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , initializing(true)
    , ui(new Ui::MainWindow)
    , settings("qdiskinfo", "qdiskinfo")
{
    ui->setupUi(this);

    locale = QLocale::system();

    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    QWidget *containerWidget = ui->scrollAreaWidgetContents;
    horizontalLayout = new QHBoxLayout(containerWidget);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui->scrollArea->setWidget(containerWidget);

    diskName = qobject_cast<QLabel *>(ui->centralwidget->findChild<QLabel*>("diskName"));
    temperatureValue = qobject_cast<QLabel *>(ui->centralwidget->findChild<QLabel*>("temperatureValueLabel"));
    healthStatusValue = qobject_cast<QLabel *>(ui->centralwidget->findChild<QLabel*>("healthStatusValueLabel"));

    firmwareLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("firmwareLineEdit"));
    serialNumberLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("serialNumberLineEdit"));
    typeLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("typeLineEdit"));
    protocolLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("protocolLineEdit"));
    deviceNodeLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("deviceNodeLineEdit"));

    totalReadsLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("totalReadsLineEdit"));
    totalWritesLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("totalWritesLineEdit"));
    rotationRateLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("rotationRateLineEdit"));
    powerOnCountLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("powerOnCountLineEdit"));
    powerOnHoursLineEdit = qobject_cast<QLineEdit *>(ui->centralwidget->findChild<QLineEdit*>("powerOnHoursLineEdit"));

    tableWidget = qobject_cast<QTableWidget *>(ui->centralwidget->findChild<QTableWidget*>("dataTable"));
    serialNumberLineEdit->setEchoMode(QLineEdit::Password);

    nextButton = ui->centralwidget->findChild<QPushButton*>("nextButton");
    prevButton = ui->centralwidget->findChild<QPushButton*>("previousButton");
    nextButton->setShortcut(QKeySequence::Forward);
    prevButton->setShortcut(QKeySequence::Back);

    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNextButtonClicked);
    connect(prevButton, &QPushButton::clicked, this, &MainWindow::onPrevButtonClicked);

    nextButton->setFocusPolicy(Qt::NoFocus);
    prevButton->setFocusPolicy(Qt::NoFocus);

    ui->actionSave_JSON->setShortcut(QKeySequence::Save);
    ui->actionQuit->setShortcut(QKeySequence::Quit);
    ui->actionRescan_Refresh->setShortcut(QKeySequence::Refresh);

    menuDevice = ui->menuDevice;
    menuDisk = ui->menuDisk;

    selfTestMenu = new QMenu(tr("Start Self Test"), this);
    menuDevice->addMenu(selfTestMenu);
    selfTestMenu->setToolTipsVisible(true);

    selfTestLogAction = new QAction(tr("Self Test Log"), this);
    menuDevice->addAction(selfTestLogAction);

    disksGroup = new QActionGroup(this);
    disksGroup->setExclusive(true);

    goodColor = QColor(Qt::green);
    cautionColor = QColor(Qt::yellow);
    badColor = QColor(Qt::red);
    naColor = QColor(Qt::gray);

    ui->actionIgnore_C4_Reallocation_Event_Count->setChecked(settings.value("IgnoreC4", true).toBool());
    ui->actionHEX->setChecked(settings.value("HEX", true).toBool());
    ui->actionUse_Fahrenheit->setChecked(settings.value("Fahrenheit", false).toBool());
    ui->actionCyclic_Navigation->setChecked(settings.value("CyclicNavigation", false).toBool());
    ui->actionUse_GB_instead_of_TB->setChecked(settings.value("UseGB", false).toBool());

    QAction *toggleEchoModeAction = serialNumberLineEdit->addAction(QIcon::fromTheme(QStringLiteral("visibility")), QLineEdit::TrailingPosition);
    connect(toggleEchoModeAction, &QAction::triggered, this, [=]() {
        if (serialNumberLineEdit->echoMode() == QLineEdit::Password) {
            serialNumberLineEdit->setEchoMode(QLineEdit::Normal);
            toggleEchoModeAction->setIcon(QIcon::fromTheme(QStringLiteral("hint")));
        } else if (serialNumberLineEdit->echoMode() == QLineEdit::Normal) {
            serialNumberLineEdit->setEchoMode(QLineEdit::Password);
            toggleEchoModeAction->setIcon(QIcon::fromTheme(QStringLiteral("visibility")));
        }
    });

    QPair<QStringList, QJsonArray> values = Utils.scanDevices(initializing);
    deviceOutputs = values.first;
    devices = values.second;
    if (!deviceOutputs.isEmpty()) {
        updateUI();
    }
    this->setFocus();
    initializing = false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onNextButtonClicked()
{
    int currentIndex = buttonGroup->buttons().indexOf(buttonGroup->checkedButton());
    int nextIndex = (currentIndex + 1) % buttonGroup->buttons().size();
    buttonGroup->buttons().at(nextIndex)->click();
    updateNavigationButtons(nextIndex);
}

void MainWindow::onPrevButtonClicked()
{
    int currentIndex = buttonGroup->buttons().indexOf(buttonGroup->checkedButton());
    int prevIndex = (currentIndex - 1 + buttonGroup->buttons().size()) % buttonGroup->buttons().size();
    buttonGroup->buttons().at(prevIndex)->click();
    updateNavigationButtons(prevIndex);
}

void MainWindow::updateNavigationButtons(int currentIndex)
{
    prevButton->setEnabled(currentIndex > 0 || (ui->actionCyclic_Navigation->isChecked() && buttonGroup->buttons().size() > 1)); // We can use setVisible if we want to mimic CrystalDiskInfo
    nextButton->setEnabled(currentIndex < buttonGroup->buttons().size() - 1 || ui->actionCyclic_Navigation->isChecked());
}

void MainWindow::updateUI()
{
    bool firstTime = true;
    globalIsNvme = false;

    for (int i = 0; i < devices.size(); ++i) {
        QJsonObject device = devices[i].toObject();
        QString deviceName = device["name"].toString();

        QString allOutput;
        if (i >= 0 && i < deviceOutputs.size()) {
            allOutput = deviceOutputs[i];
        }

        QJsonDocument localDoc = QJsonDocument::fromJson(allOutput.toUtf8());
        QJsonObject localObj = localDoc.object();

        QString modelName = localObj["model_name"].toString();
        QJsonArray attributes = localObj["ata_smart_attributes"].toObject()["table"].toArray();
        QJsonObject nvmeLog = localObj["nvme_smart_health_information_log"].toObject();
        QString temperature = "-- °C";
        bool healthPassed = localObj["smart_status"].toObject()["passed"].toBool();
        bool caution = false;
        bool bad = false;
        QString health;
        QColor healthColor;

        float diskCapacityGB = localObj.value("user_capacity").toObject().value("bytes").toDouble() / 1e9;
        QString gbSymbol = locale.formattedDataSize(1 << 30, 1, QLocale::DataSizeTraditionalFormat).split(' ')[1];
        QString tbSymbol = locale.formattedDataSize(qint64(1) << 40, 1, QLocale::DataSizeTraditionalFormat).split(' ')[1];
        QString diskCapacityString;
        int diskCapacityGbInt = static_cast<int>(diskCapacityGB);
        bool useGB = ui->actionUse_GB_instead_of_TB->isChecked();
        if (diskCapacityGbInt < 1000 || useGB) {
            diskCapacityString = locale.toString(diskCapacityGB, 'f', 1) + " " + gbSymbol;
        } else {
            diskCapacityString = QString::number(diskCapacityGbInt/1000) + " " + tbSymbol;
        }

        QString protocol = localObj["device"].toObject()["protocol"].toString();
        bool isNvme = (protocol == "NVMe");

        int temperatureInt = localObj["temperature"].toObject()["current"].toInt();
        if (temperatureInt > 0) {
            if (ui->actionUse_Fahrenheit->isChecked()) {
                int fahrenheit = static_cast<int>((temperatureInt * 9.0 / 5.0) + 32.0);
                temperature = QString::number(fahrenheit) + " °F";
            } else {
                temperature = QString::number(temperatureInt) + " °C";
            }
        }

        QVector<QPair<QString, int>> nvmeSmartOrdered;
        if (!isNvme) {
            for (const QJsonValue &attr : attributes) {
                QJsonObject attrObj = attr.toObject();
                if ((attrObj["id"] == 5 || attrObj["id"] == 197 || attrObj["id"] == 198 || (attrObj["id"] == 196 && !(ui->actionIgnore_C4_Reallocation_Event_Count->isChecked()))) && attrObj["raw"].toObject()["value"].toDouble()) {
                    caution = true;
                }
                if (attrObj["thresh"].toInt() && (attrObj["value"].toInt() < attrObj["thresh"].toInt())) {
                    bad = true;
                }
            }
        } else {
            JsonParser parser;
            nvmeSmartOrdered = parser.parse(allOutput);
            int row = 1;
            for (const QPair<QString, int> &pair : std::as_const(nvmeSmartOrdered)) {
                QString id = QString("%1").arg(row, 2, 16, QChar('0')).toUpper();
                int raw = pair.second;
                if (id == "01" && raw) {
                    bad = true;
                } else if (id == "03") {
                    int availableSpareThreshold = nvmeSmartOrdered.at(3).second;
                    if (availableSpareThreshold > 100) { // Thx to crystaldiskinfo for these workarounds
                        ;
                    } else if (raw == 0 && availableSpareThreshold == 0) {
                        ;
                    } else if (raw < availableSpareThreshold) {
                        bad = true;
                    } else if (availableSpareThreshold != 100 && (raw == availableSpareThreshold)) {
                        caution = true;
                    }
                } else if (id == "05" && raw >= 90) { // Make this configurable, currently hardcoded to 10%
                    caution = true;
                }

                ++row;
            }
        }

        if (healthPassed && !caution && !bad) {
            health = tr("Good");
            healthColor = goodColor;
        } else if (healthPassed && caution && !bad) {
            health = tr("Caution");
            healthColor = cautionColor;
        } else if ((bad || !healthPassed) && !modelName.isEmpty()){
            health = tr("Bad");
            healthColor = badColor;
        } else {
            health = tr("Unknown");
            healthColor = naColor;
        }

        CustomButton *button = new CustomButton(health, temperature, deviceName, healthColor, this);
        button->setToolTip(tr("Disk") + " " + QString::number(i) + " : " +  modelName + " : " + diskCapacityString);

        buttonGroup->addButton(button);
        horizontalLayout->addWidget(button);

        QAction *diskAction = new QAction("(" + QString::number(i+1) + ") " + modelName + " " + diskCapacityString, this);
        diskAction->setCheckable(true);
        menuDisk->addAction(diskAction);
        disksGroup->addAction(diskAction);

        button->setCheckable(true);
        button->setAutoExclusive(true);

        int buttonIndex = buttonGroup->buttons().indexOf(button);

        auto updateWindow = [=]() {
            if (isNvme) {
                populateWindow(localObj, health, nvmeSmartOrdered);
            } else {
                populateWindow(localObj, health);
            }
            updateNavigationButtons(buttonIndex);
        };

        connect(button, &QPushButton::clicked, this, [=]() {
            updateWindow();
            disksGroup->actions().at(buttonIndex)->setChecked(true);
        });

        connect(diskAction, &QAction::triggered, this, [=]() {
            updateWindow();
        });

        if (firstTime) {
            globalObj = localObj;
            globalHealth = health;
            button->setChecked(true);
            diskAction->setChecked(true);
            firstTime = false;
            globalIsNvme = isNvme;
            if (isNvme) {
                globalNvmeSmartOrdered = nvmeSmartOrdered;
            }
        }
    }

    buttonStretch = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addSpacerItem(buttonStretch);

    if (globalIsNvme) {
        populateWindow(globalObj, globalHealth, globalNvmeSmartOrdered);
    } else {
        populateWindow(globalObj, globalHealth);
    }

    updateNavigationButtons(buttonGroup->buttons().indexOf(buttonGroup->checkedButton()));
}

void MainWindow::populateWindow(const QJsonObject &localObj, const QString &health, const QVector<QPair<QString, int>>& nvmeLogOrdered)
{
    QJsonArray attributes = localObj["ata_smart_attributes"].toObject()["table"].toArray();
    QJsonObject pollingMinutes = localObj["ata_smart_data"].toObject()["self_test"].toObject()["polling_minutes"].toObject();
    QJsonObject nvmeLog = localObj["nvme_smart_health_information_log"].toObject();
    QString modelName = localObj["model_name"].toString();
    QString firmwareVersion = localObj["firmware_version"].toString();
    float diskCapacityGB = localObj.value("user_capacity").toObject().value("bytes").toDouble() / 1e9;
    int diskCapacityGbInt = static_cast<int>(diskCapacityGB);
    int temperatureInt =  localObj["temperature"].toObject()["current"].toInt();
    int totalWritesInt = 0;
    int totalReadsInt = 0;
    bool useGB = ui->actionUse_GB_instead_of_TB->isChecked();

    QString gbSymbol = locale.formattedDataSize(1 << 30, 1, QLocale::DataSizeTraditionalFormat).split(' ')[1];
    QString tbSymbol = locale.formattedDataSize(qint64(1) << 40, 1, QLocale::DataSizeTraditionalFormat).split(' ')[1];
    QString diskCapacityString;
    if (diskCapacityGbInt < 1000 || useGB) {
        diskCapacityString = locale.toString(diskCapacityGB, 'f', 1) + " " + gbSymbol;
    } else {
        diskCapacityString = QString::number(diskCapacityGbInt/1000) + " " + tbSymbol;
    }

    QString totalReads;
    QString totalWrites;
    QString percentage = "";
    QString serialNumber = localObj["serial_number"].toString();
    QJsonObject deviceObj = localObj["device"].toObject();
    QString protocol = deviceObj["protocol"].toString();
    QString type = deviceObj["type"].toString();
    QString name = deviceObj["name"].toString();
    QJsonArray outputArray = localObj.value("smartctl").toObject()["output"].toArray();

    QJsonArray nvmeSelfTestsTable = localObj["nvme_self_test_log"].toObject()["table"].toArray();
    QJsonArray ataSelfTestsTable = localObj["ata_smart_self_test_log"].toObject()["standard"].toObject()["table"].toArray();

    bool isNvme = (protocol == "NVMe");
    bool nvmeHasSelfTest = false;

    auto createTablePopup = [=](QJsonArray selfTestsTable) {
        QWidget *popup = new QWidget();
        QTableWidget *tableWidget = new QTableWidget();

        tableWidget->setRowCount(selfTestsTable.size());
        tableWidget->setColumnCount(3);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->setHorizontalHeaderLabels({tr("Type"), tr("Status"), tr("Power On Hours")});

        for (int i = 0; i < selfTestsTable.size(); ++i) {
            QJsonObject entry = selfTestsTable[i].toObject();
            QTableWidgetItem *item;

            if (isNvme) {
                tableWidget->setItem(i, 0, new QTableWidgetItem(entry["self_test_code"].toObject()["string"].toString()));
                tableWidget->setItem(i, 1, new QTableWidgetItem(entry["self_test_result"].toObject()["string"].toString()));
                item = new QTableWidgetItem(QString::number(entry["power_on_hours"].toInt()));
            } else {
                tableWidget->setItem(i, 0, new QTableWidgetItem(entry["type"].toObject()["string"].toString()));
                tableWidget->setItem(i, 1, new QTableWidgetItem(entry["status"].toObject()["string"].toString()));
                item = new QTableWidgetItem(QString::number(entry["lifetime_hours"].toInt()));
            }
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tableWidget->setItem(i, 2, item);
        }

        for (int i = 0; i < tableWidget->columnCount(); ++i) {
            QTableWidgetItem *headerItem = tableWidget->horizontalHeaderItem(i);
            if (headerItem) {
                if (i == 2) {
                   headerItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                } else {
                    headerItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                }
            }
        }


        tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        for (int i = 0; i < tableWidget->columnCount(); ++i) {
            if (i != 1) {
                tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
            }
        }

        tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tableWidget->verticalHeader()->setDefaultSectionSize(31);
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(tableWidget);
        popup->setLayout(layout);
        popup->setWindowTitle(tr("Self Test Log"));
        popup->resize(400, 400);
        popup->show();
    };

    deviceJson = localObj;

    diskName->setText("<html><head/><body><p><span style='font-size:14pt; font-weight:bold;'>" + modelName + " " + diskCapacityString + "</span></p></body></html>");
    firmwareLineEdit->setText(firmwareVersion);
    serialNumberLineEdit->setText(serialNumber);
    typeLineEdit->setText(type);
    protocolLineEdit->setText(protocol);
    deviceNodeLineEdit->setText(name);

    QStringList keys = pollingMinutes.keys();
    std::sort(keys.begin(), keys.end(), [&pollingMinutes](const QString& key1, const QString& key2) {
        return pollingMinutes[key1].toInt() < pollingMinutes[key2].toInt();
    });

    int rotationRateInt = localObj["rotation_rate"].toInt(-1);
    QString rotationRate;
    if (rotationRateInt > 0) {
        rotationRate = QString::number(rotationRateInt);
    } else if (rotationRateInt == 0 || isNvme) {
        rotationRate = "---- (SSD)";
    } else {
        rotationRate = "----";
    }

    rotationRateLineEdit->setText(rotationRate);
    rotationRateLineEdit->setAlignment(Qt::AlignRight);

    int powerCycleCountInt = localObj["power_cycle_count"].toInt(-1);
    QString powerCycleCount;
    if (powerCycleCountInt >= 0) {
        powerCycleCount = QString::number(powerCycleCountInt) + " " + tr("count");
    } else {
        powerCycleCount = "Unknown";
    }

    powerOnCountLineEdit->setText(powerCycleCount);
    powerOnCountLineEdit->setAlignment(Qt::AlignRight);

    int powerOnTimeInt = localObj["power_on_time"].toObject().value("hours").toInt(-1);
    QString powerOnTime;
    if (powerOnTimeInt >= 0) {
        powerOnTime = QString::number(powerOnTimeInt) + " " + tr("hours");
    } else {
        powerOnTime = "Unknown";
    }

    powerOnHoursLineEdit->setText(powerOnTime);
    powerOnHoursLineEdit->setAlignment(Qt::AlignRight);

    if (!isNvme) {
        for (const QJsonValue &attr : attributes) {
            QJsonObject attrObj = attr.toObject();
            if (attrObj["id"] == 202) {
                if (attrObj["name"] == "Percent_Lifetime_Remain") {
                    int percentageUsed = 100 - attrObj["raw"].toObject()["value"].toInt();
                    percentage = QString::number(percentageUsed) + " %";
                }
            } else if (attrObj["id"] == 241) {
                if (attrObj["name"] == "Total_Writes_GB") {
                    int gigabytes = attrObj["raw"].toObject()["value"].toInt();
                    totalWritesInt = gigabytes;
                } else if (attrObj["name"] == "Host_Writes_32MiB") {
                    double gigabytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalWritesInt = static_cast<int>(gigabytes);
                } else if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long gigabytes = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWritesInt = static_cast<int>(gigabytes);
                } else if (attrObj["name"] == "Host_Writes_GiB" || attrObj["name"] == "Lifetime_Writes_GiB") {
                    double gibibytes = attrObj["raw"].toObject()["value"].toDouble();
                    double bytesPerGiB = static_cast<double>(1ULL << 30);
                    double bytesPerGB = 1e9;
                    double conversionFactor = bytesPerGiB / bytesPerGB;
                    double gigabytes = gibibytes * conversionFactor;
                    totalWritesInt = static_cast<int>(gigabytes);
                }
            } else if (attrObj["id"] == 242) {
                if (attrObj["name"] == "Total_Reads_GB") {
                    int gigabytes = attrObj["raw"].toObject()["value"].toInt();
                    totalReadsInt = gigabytes;
                } else if (attrObj["name"] == "Host_Reads_32MiB") {
                    double gigabytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalReadsInt = static_cast<int>(gigabytes);
                } else if (attrObj["name"] == "Total_LBAs_Read") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaRead = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long gigabytes = (lbaRead * logicalBlockSize) / oneGB;
                    totalReadsInt = static_cast<int>(gigabytes);
                } else if (attrObj["name"] == "Host_Reads_GiB" || attrObj["name"] == "Lifetime_Reads_GiB") {
                    double gibibytes = attrObj["raw"].toObject()["value"].toDouble();
                    double bytesPerGiB = static_cast<double>(1ULL << 30);
                    double bytesPerGB = 1e9;
                    double conversionFactor = bytesPerGiB / bytesPerGB;
                    double gigabytes = gibibytes * conversionFactor;
                    totalReadsInt = static_cast<int>(gigabytes);
                }
            } else if (attrObj["id"] == 246) { // MX500
                if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long gigabytes = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWritesInt = static_cast<int>(gigabytes);
                }
            } else if (attrObj["name"] == "Remaining_Lifetime_Perc") {
                int percentageUsed = attrObj["raw"].toObject()["value"].toInt();
                percentage = QString::number(percentageUsed) + " %";
            } else if (attrObj["name"] == "Media_Wearout_Indicator" || attrObj["name"] == "SSD_Life_Left") {
                int percentageUsed = attrObj["value"].toInt();
                percentage = QString::number(percentageUsed) + " %";
            }
        }
        if (percentage.isEmpty() && rotationRate == "---- (SSD)") { // Workaround for some drives which have this and another attribute
            for (const QJsonValue &attr : attributes) {
                QJsonObject attrObj = attr.toObject();
                if (attrObj["name"] == "Wear_Leveling_Count") {
                    int percentageUsed = attrObj["value"].toInt();
                    percentage = QString::number(percentageUsed) + " %";
                    break;
                }
            }
        }
    } else {
        for (auto smartItem = nvmeLog.begin(); smartItem != nvmeLog.end(); ++smartItem) {
            QString key = smartItem.key();
            QJsonValue value = smartItem.value();
            if (key == "data_units_written") {
                double dataUnitsWritten = value.toDouble();
                double gigabytes = (dataUnitsWritten * 512) / 1'000'000;
                totalWritesInt = static_cast<int>(gigabytes);
            } else if (key == "data_units_read") {
                double dataUnitsRead = value.toDouble();
                double gigabytes = (dataUnitsRead * 512) / 1'000'000;
                totalReadsInt = static_cast<int>(gigabytes);
            } else if (key == "percentage_used") {
                int percentageUsed = 100 - value.toInt();
                percentage = QString::number(percentageUsed) + " %";
            }
        }
    }

    if (totalReadsInt) {
        if (totalReadsInt < 1000 || useGB) {
            totalReads = QString::number(totalReadsInt) + " " + gbSymbol;
        } else {
            totalReads = QString::number(totalReadsInt/1000) + " " + tbSymbol;
        }
    } else {
        totalReads = "----";
    }

    if (totalWritesInt) {
        if (totalWritesInt < 1000 || useGB) {
            totalWrites = QString::number(totalWritesInt) + " " + gbSymbol;
        } else {
            totalWrites = QString::number(totalWritesInt/1000) + " " + tbSymbol;
        }
    } else {
        totalWrites = "----";
    }

    totalReadsLineEdit->setText(totalReads);
    totalReadsLineEdit->setAlignment(Qt::AlignRight);

    totalWritesLineEdit->setText(totalWrites);
    totalWritesLineEdit->setAlignment(Qt::AlignRight);

    int warningTemperature = 55;
    int criticalTemperature = 60;

    if (isNvme) {
        QString stringValue;
        warningTemperature = 65;
        criticalTemperature = 70;
        for (const QJsonValue &value : outputArray) {
            stringValue = value.toString();
            if (stringValue.startsWith("Optional Admin Commands")) {
                if (stringValue.contains("Self_Test")) {
                    nvmeHasSelfTest = true;
                }
            } else if (stringValue.startsWith("Warning  Comp. Temp. Threshold")) {
                int pos = stringValue.indexOf(':');
                if (pos != -1) {
                    QString thresholdStr = stringValue.mid(pos + 1).trimmed();
                    int temperature = thresholdStr.section(' ', 0, 0).toInt();
                    if (temperature > 0) {
                        warningTemperature = temperature;
                    }
                }
            } else if (stringValue.startsWith("Critical Comp. Temp. Threshold")) {
                int pos = stringValue.indexOf(':');
                if (pos != -1) {
                    QString thresholdStr = stringValue.mid(pos + 1).trimmed();
                    int temperature = thresholdStr.section(' ', 0, 0).toInt();
                    if (temperature > 0) {
                        criticalTemperature = temperature;
                    }
                }
            }
        }
    }

    if (temperatureInt > warningTemperature) { // TODO: Let the user set an alarm temp.
        temperatureValue->setStyleSheet("background-color: " + badColor.name() + ";");
    } else if ((temperatureInt < criticalTemperature) && (temperatureInt > warningTemperature)){
        temperatureValue->setStyleSheet("background-color: " + cautionColor.name() + ";");
    } else if (temperatureInt == 0) {
        temperatureValue->setStyleSheet("background-color: " + naColor.name() + ";");
    } else {
        temperatureValue->setStyleSheet("background-color: " + goodColor.name() + ";");
    }

    QString labelStyle = "font-size:12pt; font-weight:700; color:black";

    if (temperatureInt > 0) {
        if (ui->actionUse_Fahrenheit->isChecked()) {
            int fahrenheit = static_cast<int>((temperatureInt * 9.0 / 5.0) + 32.0);
            temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + QString::number(fahrenheit) + " °F</span></p></body></html>");
        } else {
            temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + QString::number(temperatureInt) + " °C</span></p></body></html>");
        }
    } else {
        temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + "-- °C" + "</span></p></body></html>");
    }

    temperatureValue->setAlignment(Qt::AlignCenter);

    if (health == tr("Bad")) {
        healthStatusValue->setStyleSheet("background-color: " + badColor.name() + ";");
    } else if (health == tr("Caution")){
        healthStatusValue->setStyleSheet("background-color: " + cautionColor.name() + ";");
    } else if (health == tr("Good")) {
        healthStatusValue->setStyleSheet("background-color: " + goodColor.name() + ";");
    } else {
        healthStatusValue->setStyleSheet("background-color: " + naColor.name() + ";");
    }

    QString percentageText = "";
    if (!percentage.isEmpty()) {
        percentageText = "<br/>" + percentage;
    }
    healthStatusValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + health + percentageText + "</span></p></body></html>");
    healthStatusValue->setAlignment(Qt::AlignCenter);

    if (protocol != "NVMe") {
        addSmartAttributesTable(attributes);
        selfTestMenu->clear();

        if (keys.isEmpty()) {
            selfTestMenu->setDisabled(true);
        } else {
            selfTestMenu->setEnabled(true);
        }

        int i = 0;
        for (const QString& key : keys) {
            QString minutes = QString::number(static_cast<int>(pollingMinutes[key].toInt()));
            QString keyTranslated;
            if (key == "short") {
                keyTranslated = tr("Short");
            } else if (key == "conveyance") {
                keyTranslated = tr("Conveyance");
            } else if (key == "extended") {
                keyTranslated = tr("Extended");
            } else {
                keyTranslated = key;
                keyTranslated[0] = keyTranslated[0].toUpper();
            }
            QString actionLabel = keyTranslated + " (" + minutes + " " + tr("Min.)");
            QAction *action = new QAction(actionLabel, this);
            selfTestMenu->addAction(action);

            QString mode;
            if (key == "extended") {
                mode = "long";
            } else {
                mode = key;
            }

            connect(action, &QAction::triggered, this, [this, mode, name, minutes]() {
                Utils.selfTestHandler(mode, name, minutes);
            });

            i++;
        }

        if (ataSelfTestsTable.isEmpty()) {
            selfTestLogAction->setDisabled(true);
        } else {
            selfTestLogAction->setEnabled(true);
            selfTestLogAction->disconnect();
            connect(selfTestLogAction, &QAction::triggered, this, [=]() {
                createTablePopup(ataSelfTestsTable);
            });
        }
    } else {
        addNvmeLogTable(nvmeLogOrdered);

        selfTestMenu->clear();

        if (nvmeHasSelfTest) {
            selfTestMenu->setEnabled(true);
        } else {
            selfTestMenu->setDisabled(true);
        }

        QAction *actionShort = new QAction(tr("Short"), this);
        selfTestMenu->addAction(actionShort);
        connect(actionShort, &QAction::triggered, this, [this, mode = "short", name, minutes = "0"]() {
            Utils.selfTestHandler(mode, name, minutes);
        });

        QAction *actionLong = new QAction(tr("Extended"), this);
        selfTestMenu->addAction(actionLong);
        connect(actionLong , &QAction::triggered, this, [this, mode = "long", name, minutes = "0"]() {
            Utils.selfTestHandler(mode, name, minutes);
        });

        if (nvmeSelfTestsTable.isEmpty()) {
            selfTestLogAction->setDisabled(true);
        } else {
            selfTestLogAction->setEnabled(true);
            selfTestLogAction->disconnect();
            connect(selfTestLogAction, &QAction::triggered, this, [=]() {
                createTablePopup(nvmeSelfTestsTable);
            });
        }
    }
}

void MainWindow::addNvmeLogTable(const QVector<QPair<QString, int>>& nvmeLogOrdered)
{
    QString warningMessage = "";

    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"", tr("ID"), tr("Attribute Name"), tr("Raw Values")});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setItemDelegateForColumn(0, new StatusDot(tableWidget));
    tableWidget->setRowCount(nvmeLogOrdered.size());

    tableWidget->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    int row = 0;
    for (const QPair<QString, int> &pair : std::as_const(nvmeLogOrdered)) {
        QString id = QString("%1").arg(row + 1, 2, 16, QChar('0')).toUpper();

        QString key = pair.first;
        QString name = key.replace("_", " ");
        name = Utils.toTitleCase(name);

        int rawInt = pair.second;
        QString raw = QString::number(rawInt);
        if ((ui->actionHEX->isChecked())) {
            raw = QString("%1").arg(raw.toUInt(nullptr), 14, 16, QChar('0')).toUpper();
        }

        QColor statusColor;

        if (id == "01" && rawInt) {
            statusColor = badColor;
            if (rawInt == 1) { // Still need to figure out if this is DEC or HEX in JSON
                warningMessage = tr("Available spare capacity has fallen below the threshold"); // THX to CrystalDiskInfo for the messages
            } else if (rawInt == 2) {
                warningMessage = tr("Temperature error (Overheat or Overcool)");
            } else if (rawInt == 4) {
                warningMessage = tr("NVM subsystem reliability has been degraded");
            } else if (rawInt == 8) {
                warningMessage = tr("Media has been placed in Read Only Mode");
            } else if (rawInt == 10) {
                warningMessage = tr("Volatile memory backup device has Failed");
            } else if (rawInt == 20) {
                warningMessage = tr("Persistent memory region has become Read-Only");
            }
        } else if (id == "03") {
            int availableSpareThreshold = nvmeLogOrdered.at(3).second;
            if (availableSpareThreshold > 100) { // Thx to crystaldiskinfo for these workarounds
                statusColor = goodColor;
            } else if (rawInt == 0 && availableSpareThreshold == 0) {
                statusColor = goodColor;
            } else if (rawInt < availableSpareThreshold) {
                statusColor = badColor;
            } else if (availableSpareThreshold != 100 && (rawInt == availableSpareThreshold)) {
                statusColor = cautionColor;
            } else {
                statusColor = goodColor;
            }
        } else if (id == "05" && rawInt >= 90) { // Make this configurable, currently hardcoded to 10%
            statusColor = cautionColor;
        } else {
            statusColor = goodColor;
        }

        QTableWidgetItem *statusItem = new QTableWidgetItem();
        statusItem->setData(Qt::BackgroundRole, QVariant(statusColor));

        QTableWidgetItem *idItem = new QTableWidgetItem(id);
        idItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);

        QTableWidgetItem *rawItem = new QTableWidgetItem(raw);
        rawItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        tableWidget->setItem(row, 0, statusItem);
        tableWidget->setItem(row, 1, idItem);
        tableWidget->setItem(row, 2, nameItem);
        tableWidget->setItem(row, 3, rawItem);

        ++row;
    }

    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int i = 0; i < tableWidget->columnCount(); ++i) {
        if (i != 2) {
            tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    }

    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tableWidget->verticalHeader()->setDefaultSectionSize(31);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    if (!warningMessage.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Critical Warning"), warningMessage);
    }
}

void MainWindow::addSmartAttributesTable(const QJsonArray &attributes)
{
    tableWidget->setColumnCount(7);
    tableWidget->setHorizontalHeaderLabels({"", tr("ID"), tr("Attribute Name"), tr("Current"), tr("Worst"), tr("Threshold"), tr("Raw Values")});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setItemDelegateForColumn(0, new StatusDot(tableWidget));
    tableWidget->setRowCount(attributes.size());

    for (int i = 0; i < tableWidget->columnCount(); ++i) {
        QTableWidgetItem *headerItem = tableWidget->horizontalHeaderItem(i);
        if (headerItem) {
            if (i == 2) {
                headerItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            } else if (i > 2) {
                headerItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
    }

    int row = 0;
    for (const QJsonValue &attr : attributes) {
        QJsonObject attrObj = attr.toObject();
        QString id = QString("%1").arg(attrObj["id"].toInt(), 2, 16, QChar('0')).toUpper();
        QString name = attrObj["name"].toString().replace("_", " ");
        int value = attrObj["value"].toInt();
        int worst = attrObj["worst"].toInt();
        int thresh = attrObj["thresh"].toInt();
        QString rawString = attrObj["raw"].toObject()["string"].toString();
        QString rawHEX = rawString;

        int spaceIndex = rawHEX.indexOf(' ');
        if (spaceIndex != -1) {
            rawHEX = rawHEX.left(spaceIndex);
        }

        if (rawHEX.startsWith("0x") && rawHEX.length() == 14) {
            rawHEX = rawHEX.mid(2).toUpper();
        } else {
            rawHEX = QString("%1").arg(rawHEX.toUInt(nullptr), 12, 16, QChar('0')).toUpper();
        }

        QColor statusColor;
        if (thresh && (value < thresh)) {
            statusColor = badColor;
        } else if ((id == "05" || id == "C5" || id == "C6" || (id == "C4" && !(ui->actionIgnore_C4_Reallocation_Event_Count->isChecked()))) && (rawHEX != "000000000000")) {
            statusColor = cautionColor;
        } else {
            statusColor = goodColor;
        }

        QTableWidgetItem *statusItem = new QTableWidgetItem();
        statusItem->setData(Qt::BackgroundRole, QVariant(statusColor));

        QTableWidgetItem *idItem = new QTableWidgetItem(id);
        idItem->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);

        QTableWidgetItem *valueItem = new QTableWidgetItem(QString::number(value));
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *worstItem = new QTableWidgetItem(QString::number(worst));
        worstItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *threshItem = new QTableWidgetItem(QString::number(thresh));
        threshItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QTableWidgetItem *rawItem;
        if (ui->actionHEX->isChecked()) {
            rawItem = new QTableWidgetItem(rawHEX);
        } else {
            rawItem = new QTableWidgetItem(rawString);
        }
        rawItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        tableWidget->setItem(row, 0, statusItem);
        tableWidget->setItem(row, 1, idItem);
        tableWidget->setItem(row, 2, nameItem);
        tableWidget->setItem(row, 3, valueItem);
        tableWidget->setItem(row, 4, worstItem);
        tableWidget->setItem(row, 5, threshItem);
        tableWidget->setItem(row, 6, rawItem);

        ++row;
    }

    tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    for (int i = 0; i < tableWidget->columnCount(); ++i) {
        if (i != 2) {
            tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        }
    }

    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tableWidget->verticalHeader()->setDefaultSectionSize(31);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionSave_JSON_triggered()
{
    if (deviceJson.isEmpty()) {
        QMessageBox::information(this, tr("Empty JSON"),
                                 tr("The JSON is empty"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save JSON"), "",
                                                    tr("JSON (*.json);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file for writing"),
                                     file.errorString());
            return;
        }

        QJsonDocument doc(deviceJson);
        QByteArray jsonData = doc.toJson();

        file.write(jsonData);
        file.close();
    }
}

void MainWindow::on_actionGitHub_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/edisionnano/QDiskInfo"));
}

void MainWindow::on_actionRescan_Refresh_triggered()
{
    QPair<QStringList, QJsonArray> values = Utils.scanDevices(initializing);
    deviceOutputs = values.first;
    devices = values.second;
    if (!deviceOutputs.isEmpty()) {
        Utils.clearButtonGroup(buttonGroup, horizontalLayout, buttonStretch, menuDisk);
        updateUI();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QString message = tr("An ATA and NVMe S.M.A.R.T. data viewer for Linux") + "\n";
    message += tr("Licensed under the GNU G.P.L. Version 3") + "\n";
    message += tr("Made by Samantas5855") + "\n";
    message += tr("Version") + " " + QString::number(PROJECT_VERSION_MAJOR) + "." + QString::number(PROJECT_VERSION_MINOR);

    QMessageBox::about(this, tr("About QDiskInfo"), message);
}

void MainWindow::on_actionIgnore_C4_Reallocation_Event_Count_toggled(bool enabled)
{
    settings.setValue("IgnoreC4", ui->actionIgnore_C4_Reallocation_Event_Count->isChecked());
    if (!initializing) {
        Utils.clearButtonGroup(buttonGroup, horizontalLayout, buttonStretch, menuDisk);
        updateUI();
    }
}

void MainWindow::on_actionHEX_toggled(bool enabled)
{
    settings.setValue("HEX", ui->actionHEX->isChecked());
    if (!initializing) {
        Utils.clearButtonGroup(buttonGroup, horizontalLayout, buttonStretch, menuDisk);
        updateUI();
    }
}

void MainWindow::on_actionUse_Fahrenheit_toggled(bool enabled)
{
    settings.setValue("Fahrenheit", ui->actionUse_Fahrenheit->isChecked());
    if (!initializing) {
        Utils.clearButtonGroup(buttonGroup, horizontalLayout, buttonStretch, menuDisk);
        updateUI();
    }
}

void MainWindow::on_actionCyclic_Navigation_toggled(bool cyclicNavigation)
{
    settings.setValue("CyclicNavigation", ui->actionCyclic_Navigation->isChecked());
    int currentIndex = buttonGroup->buttons().indexOf(buttonGroup->checkedButton());
    updateNavigationButtons(currentIndex);
}

void MainWindow::on_actionUse_GB_instead_of_TB_toggled(bool gigabytes)
{
    settings.setValue("UseGB", ui->actionUse_GB_instead_of_TB->isChecked());
    if (!initializing) {
        Utils.clearButtonGroup(buttonGroup, horizontalLayout, buttonStretch, menuDisk);
        updateUI();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::ForwardButton && nextButton->isEnabled()) {
        onNextButtonClicked();
    } else if (event->button() == Qt::BackButton && prevButton->isEnabled()) {
        onPrevButtonClicked();
    }
}
