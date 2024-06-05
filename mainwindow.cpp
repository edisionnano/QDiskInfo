#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    horizontalLayout = ui->horizontalLayout;

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

    connect(nextButton, &QPushButton::clicked, this, &MainWindow::onNextButtonClicked);
    connect(prevButton, &QPushButton::clicked, this, &MainWindow::onPrevButtonClicked);

    nextButton->setFocusPolicy(Qt::NoFocus);
    prevButton->setFocusPolicy(Qt::NoFocus);

    goodColor = QColor(Qt::green);
    cautionColor = QColor(Qt::yellow);
    badColor = QColor(Qt::red);
    naColor = QColor(Qt::gray);

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

    scanDevices();

    this->setFocus();
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
    prevButton->setEnabled(currentIndex > 0); // We can use setVisible if we want to mimic CrystalDiskInfo
    nextButton->setEnabled(currentIndex < buttonGroup->buttons().size() - 1);
}

void MainWindow::scanDevices()
{
    QString output = getSmartctlOutput({"--scan", "--json"}, false);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObj = doc.object();
    QJsonArray devices = jsonObj["devices"].toArray();
    QStringList commandList;

    for (const QJsonValue &value : devices) {
        QJsonObject device = value.toObject();
        QString deviceName = device["name"].toString();
        commandList.append(QString("smartctl --all --json %1").arg(deviceName));
    }
    QString command = commandList.join(" ; ");

    QString allDevicesOutput = getSmartctlOutput({"sh", "-c", command}, true);

    QStringList deviceOutputs;
    int startIndex = 0;
    int endIndex = 0;

    while ((endIndex = allDevicesOutput.indexOf(QRegExp("\\}\\n\\{"), startIndex)) != -1) {
        ++endIndex;
        QString jsonFragment = allDevicesOutput.mid(startIndex, endIndex - startIndex);
        deviceOutputs.append(jsonFragment);
        startIndex = endIndex;
    }

    if (startIndex < allDevicesOutput.size()) {
        QString jsonFragment = allDevicesOutput.mid(startIndex);
        deviceOutputs.append(jsonFragment);
    }

    QJsonObject globalObj;
    QString globalHealth;
    QVector<QPair<QString, int>> globalNvmeSmartOrdered;
    bool firstTime = true;
    bool globalIsNvme = false;

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

        QString protocol = localObj["device"].toObject()["protocol"].toString();
        bool isNvme = (protocol == "NVMe");

        int temperatureInt = localObj["temperature"].toObject()["current"].toInt();
        if (temperatureInt > 0) {
            temperature = QString::number(temperatureInt) + " °C";
        }

        QVector<QPair<QString, int>> nvmeSmartOrdered;
        if (!isNvme) {
            for (const QJsonValue &attr : attributes) {
                QJsonObject attrObj = attr.toObject();
                if ((attrObj["id"] == 5 || attrObj["id"] == 197 || attrObj["id"] == 198) && attrObj["raw"].toObject()["value"].toInt()) {
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
            for (const QPair<QString, int> &pair : qAsConst(nvmeSmartOrdered)) {
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
            health = "Good";
            healthColor = goodColor;
        } else if (healthPassed && caution && !bad) {
            health = "Caution";
            healthColor = cautionColor;
        } else if ((bad || !healthPassed) && !modelName.isEmpty()){
            health = "Bad";
            healthColor = badColor;
        } else {
            health = "Unknown";
            healthColor = naColor;
        }

        CustomButton *button = new CustomButton(health, deviceName, temperature, healthColor, this);

        buttonGroup->addButton(button);
        horizontalLayout->addWidget(button);

        button->setCheckable(true);
        button->setAutoExclusive(true);

        connect(button, &QPushButton::clicked, this, [=]() {
            if (isNvme) {
                populateWindow(localObj, health, nvmeSmartOrdered);
            } else {
                populateWindow(localObj, health);
            }
            updateNavigationButtons(buttonGroup->buttons().indexOf(button));
        });

        if (firstTime) {
            globalObj = localObj;
            globalHealth = health;
            button->setChecked(true);
            firstTime = false;
            globalIsNvme = isNvme;
            if (isNvme) {
                globalNvmeSmartOrdered = nvmeSmartOrdered;
            }
        }
    }
    horizontalLayout->addStretch();

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
    QJsonObject nvmeLog = localObj["nvme_smart_health_information_log"].toObject();
    QString modelName = localObj["model_name"].toString();
    QString firmwareVersion = localObj["firmware_version"].toString();
    float userCapacityGB = localObj.value("user_capacity").toObject().value("bytes").toDouble() / 1e9;
    int temperatureInt =  localObj["temperature"].toObject()["current"].toInt();
    QString userCapacityString = QString::number(static_cast<int>(userCapacityGB)) + "." + QString::number(static_cast<int>((userCapacityGB - static_cast<int>(userCapacityGB)) * 10)) + " GB";
    QString totalReads = "----";
    QString totalWrites = "----";
    QString percentage = "";
    QString serialNumber = localObj["serial_number"].toString();
    QJsonObject deviceObj = localObj["device"].toObject();
    QString protocol = deviceObj["protocol"].toString();
    QString type = deviceObj["type"].toString();
    QString name = deviceObj["name"].toString();

    bool isNvme = (protocol == "NVMe");

    diskName->setText("<html><head/><body><p><span style='font-size:14pt; font-weight:bold;'>" + modelName + " " + userCapacityString + "</span></p></body></html>");
    firmwareLineEdit->setText(firmwareVersion);
    serialNumberLineEdit->setText(serialNumber);
    typeLineEdit->setText(type);
    protocolLineEdit->setText(protocol);
    deviceNodeLineEdit->setText(name);

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
        powerCycleCount = QString::number(powerCycleCountInt) + " count";
    } else {
        powerCycleCount = "Unknown";
    }

    powerOnCountLineEdit->setText(powerCycleCount);
    powerOnCountLineEdit->setAlignment(Qt::AlignRight);

    int powerOnTimeInt = localObj["power_on_time"].toObject().value("hours").toInt(-1);
    QString powerOnTime;
    if (powerOnTimeInt >= 0) {
        powerOnTime = QString::number(powerOnTimeInt) + " hours";
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
                    totalWrites = QString::number(attrObj["raw"].toObject()["value"].toInt()) + " GB";
                } else if (attrObj["name"] == "Host_Writes_32MiB") {
                    double gigabytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalWrites = QString::number(static_cast<int>(gigabytes)) + " GB";
                } else if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbWritten = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWrites = QString::number(static_cast<int>(totalGbWritten)) + " GB";
                } else if (attrObj["name"] == "Host_Writes_GiB" || attrObj["name"] == "Lifetime_Writes_GiB") {
                    double gibibytes = attrObj["raw"].toObject()["value"].toDouble();
                    double bytesPerGiB = static_cast<double>(1ULL << 30);
                    double bytesPerGB = 1e9;
                    double conversionFactor = bytesPerGiB / bytesPerGB;
                    double gigabytes = gibibytes * conversionFactor;
                    totalWrites = QString::number(static_cast<int>(gigabytes)) + " GB";
                }
            } else if (attrObj["id"] == 242) {
                if (attrObj["name"] == "Total_Reads_GB") {
                    totalReads = QString::number(attrObj["raw"].toObject()["value"].toInt()) + " GB";
                } else if (attrObj["name"] == "Host_Reads_32MiB") {
                    double gigabytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalReads = QString::number(static_cast<int>(gigabytes)) + " GB";
                } else if (attrObj["name"] == "Total_LBAs_Read") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaRead = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbRead = (lbaRead * logicalBlockSize) / oneGB;
                    totalReads = QString::number(static_cast<int>(totalGbRead)) + " GB";
                } else if (attrObj["name"] == "Host_Reads_GiB" || attrObj["name"] == "Lifetime_Reads_GiB") {
                    double gibibytes = attrObj["raw"].toObject()["value"].toDouble();
                    double bytesPerGiB = static_cast<double>(1ULL << 30);
                    double bytesPerGB = 1e9;
                    double conversionFactor = bytesPerGiB / bytesPerGB;
                    double gigabytes = gibibytes * conversionFactor;
                    totalReads = QString::number(static_cast<int>(gigabytes)) + " GB";
                }
            } else if (attrObj["id"] == 246) { // MX500
                if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbWritten = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWrites = QString::number(static_cast<int>(totalGbWritten)) + " GB";
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
                double totalGbWritten = (dataUnitsWritten * 512) / 1'000'000;
                totalWrites = QString::number(static_cast<int>(totalGbWritten)) + " GB";
            } else if (key == "data_units_read") {
                double dataUnitsRead = value.toDouble();
                double totalGbRead = (dataUnitsRead * 512) / 1'000'000;
                totalReads = QString::number(static_cast<int>(totalGbRead)) + " GB";
            } else if (key == "percentage_used") {
                int percentageUsed = 100 - value.toInt();
                percentage = QString::number(percentageUsed) + " %";
            }
        }
    }

    if (totalReads == "0 GB") {
        totalReads = "----";
    }

    if (totalWrites == "0 GB") {
        totalWrites = "----";
    }

    totalReadsLineEdit->setText(totalReads);
    totalReadsLineEdit->setAlignment(Qt::AlignRight);

    totalWritesLineEdit->setText(totalWrites);
    totalWritesLineEdit->setAlignment(Qt::AlignRight);

    if (temperatureInt > 55) {
        temperatureValue->setStyleSheet("background-color: " + badColor.name() + ";");
    } else if ((temperatureInt < 55) && (temperatureInt > 50)){
        temperatureValue->setStyleSheet("background-color: " + cautionColor.name() + ";");
    } else if (temperatureInt == 0) {
        temperatureValue->setStyleSheet("background-color: " + naColor.name() + ";");
    } else {
        temperatureValue->setStyleSheet("background-color: " + goodColor.name() + ";");
    }

    QString labelStyle = "font-size:12pt; font-weight:700; color:black";

    if (temperatureInt > 0) {
        temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + QString::number(temperatureInt) + " °C</span></p></body></html>");
    } else {
        temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + "-- °C" + "</span></p></body></html>");
    }

    temperatureValue->setAlignment(Qt::AlignCenter);

    if (health == "Bad") {
        healthStatusValue->setStyleSheet("background-color: " + badColor.name() + ";");
    } else if (health == "Caution"){
        healthStatusValue->setStyleSheet("background-color: " + cautionColor.name() + ";");
    } else if (health == "Good") {
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
    } else {
        addNvmeLogTable(nvmeLogOrdered);
    }
}

void MainWindow::addNvmeLogTable(const QVector<QPair<QString, int>>& nvmeLogOrdered)
{
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"", "ID", "Attribute Name", "Raw Values"});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setItemDelegateForColumn(0, new StatusDot(tableWidget));
    tableWidget->setRowCount(nvmeLogOrdered.size());

    for (int i = 0; i < tableWidget->columnCount(); ++i) {
        QTableWidgetItem *headerItem = tableWidget->horizontalHeaderItem(i);
        if (headerItem) {
            if (i == 2) {
                headerItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            } else if (i == 3) {
                headerItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
    }

    int row = 0;
    for (const QPair<QString, int> &pair : qAsConst(nvmeLogOrdered)) {
        QString id = QString("%1").arg(row + 1, 2, 16, QChar('0')).toUpper();

        QString key = pair.first;
        QString name = key.replace("_", " ");
        name = toTitleCase(name);

        int rawInt = pair.second;
        QString raw = QString::number(rawInt);
        raw = QString("%1").arg(raw.toUInt(nullptr), 14, 16, QChar('0')).toUpper();

        QColor statusColor;

        if (id == "01" && rawInt) {
            statusColor = badColor;
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
}

void MainWindow::addSmartAttributesTable(const QJsonArray &attributes)
{
    tableWidget->setColumnCount(7);
    tableWidget->setHorizontalHeaderLabels({"", "ID", "Attribute Name", "Current", "Worst", "Threshold", "Raw Values"});
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
        QString raw = attrObj["raw"].toObject()["string"].toString();

        int spaceIndex = raw.indexOf(' ');
        if (spaceIndex != -1) {
            raw = raw.left(spaceIndex);
        }

        if (raw.startsWith("0x") && raw.length() == 14) {
            raw = raw.mid(2).toUpper();
        } else {
            raw = QString("%1").arg(raw.toUInt(nullptr), 12, 16, QChar('0')).toUpper();
        }

        QColor statusColor;
        if (thresh && (value < thresh)) {
            statusColor = badColor;
        } else if ((id == "05" || id == "C5" || id == "C6") && (raw != "000000000000")) {
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

        QTableWidgetItem *rawItem = new QTableWidgetItem(raw);
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

    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

QString MainWindow::getSmartctlOutput(const QStringList &arguments, bool root)
{
    QProcess process;
    QString command;
    QStringList commandArgs;
    if (root) {
        command = "pkexec";
        commandArgs = QStringList();
    } else {
        command = "smartctl";
        commandArgs = QStringList();
    }

    commandArgs.append(arguments);
    process.start(command, commandArgs);
    process.waitForFinished();
    return process.readAllStandardOutput();
}

QString MainWindow::toTitleCase(const QString& sentence) {
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

void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}

