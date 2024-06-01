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

    tableWidget = qobject_cast<QTableWidget *>(ui->centralwidget->findChild<QTableWidget*>("dataTable"));;
    serialNumberLineEdit->setEchoMode(QLineEdit::Password);

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::scanDevices()
{
    QString output = getSmartctlOutput({"--scan", "--json"}, false);
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject jsonObj = doc.object();
    QJsonArray devices = jsonObj["devices"].toArray();
    QJsonObject globalObj;
    QString globalHealth;
    bool firstTime = true;

    for (const QJsonValue &value : devices) {
        QJsonObject device = value.toObject();
        QString deviceName = device["name"].toString();

        QString allOutput = getSmartctlOutput({"smartctl", "--all", "--json", deviceName}, true);

        QJsonDocument localDoc = QJsonDocument::fromJson(allOutput.toUtf8());
        QJsonObject localObj = localDoc.object();

        QJsonArray attributes = localObj["ata_smart_attributes"].toObject()["table"].toArray();
        QJsonObject nvmeLog = localObj["nvme_smart_health_information_log"].toObject();
        QString temperature = "N/A";
        bool healthPassed = localObj["smart_status"].toObject()["passed"].toBool();
        bool caution = false;
        QString health;
        QColor healthColor;

        bool isNvme = false;
        QString protocol = localObj["device"].toObject()["protocol"].toString();
        if (protocol == "NVMe") {
            isNvme = true;
        }

        int temperatureInt = localObj["temperature"].toObject()["current"].toInt();
        if (temperatureInt > 0) {
            temperature = QString::number(temperatureInt) + " °C";
        }

        if (!isNvme) {
            for (const QJsonValue &attr : attributes) {
                QJsonObject attrObj = attr.toObject();
                if (!isNvme && (attrObj["id"] == 5 || attrObj["id"] == 197 || attrObj["id"] == 198) && (attrObj["raw"].toObject()["value"].toInt() != 0)) {
                    caution = true;
                    break;
                }
            }
        }

        if (healthPassed && !caution) {
            health = "Good";
            healthColor = Qt::green;
        } else if (healthPassed && caution) {
            health = "Caution";
            healthColor = Qt::yellow;
        } else {
            health = "Bad";
            healthColor = Qt::red;
        }

        CustomButton *button = new CustomButton(health, deviceName, temperature, healthColor, this);

        buttonGroup->addButton(button);
        horizontalLayout->addWidget(button);

        button->setCheckable(true);
        button->setAutoExclusive(true);

        connect(button, &QPushButton::clicked, this, [=]() {
            populateWindow(localObj, health);
        });

        if (firstTime) {
            globalObj = localObj;
            globalHealth = health;
            button->setChecked(true);
            firstTime = false;
        }
    }
    horizontalLayout->addStretch();
    populateWindow(globalObj, globalHealth);
}

void MainWindow::populateWindow(const QJsonObject &localObj, const QString &health)
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
    QString serialNumber = localObj["serial_number"].toString();
    QJsonObject deviceObj = localObj["device"].toObject();
    QString protocol = deviceObj["protocol"].toString();
    QString type = deviceObj["type"].toString();
    QString name = deviceObj["name"].toString();

    bool isNvme = false;
    if (protocol == "NVMe") {
        isNvme = true;
    }

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
        for (const QJsonValue &attr : attributes) { //Need different logic for NVMe
            QJsonObject attrObj = attr.toObject();
            if (attrObj["id"] == 241) {
                if (attrObj["name"] == "Total_Writes_GB") {
                    totalWrites = QString::number(attrObj["raw"].toObject()["value"].toInt()) + " GB";
                } else if (attrObj["name"] == "Host_Writes_32MiB") {
                    double gibibytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalWrites = QString::number(static_cast<int>(gibibytes)) + " GB";
                } else if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbWritten = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWrites = QString::number(static_cast<int>(totalGbWritten)) + " GB";
                }
            } else if (attrObj["id"] == 242) {
                if (attrObj["name"] == "Total_Reads_GB") {
                    totalReads = QString::number(attrObj["raw"].toObject()["value"].toInt()) + " GB";
                } else if (attrObj["name"] == "Host_Reads_32MiB") {
                    double gibibytes = (attrObj["raw"].toObject()["value"].toInt() * 32 * 1024.0 * 1024.0) / 1e9;
                    totalReads = QString::number(static_cast<int>(gibibytes)) + " GB";
                } else if (attrObj["name"] == "Total_LBAs_Read") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaRead = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbRead = (lbaRead * logicalBlockSize) / oneGB;
                    totalReads = QString::number(static_cast<int>(totalGbRead)) + " GB";
                }
            } else if (attrObj["id"] == 246) { // MX500
                if (attrObj["name"] == "Total_LBAs_Written") {
                    unsigned int logicalBlockSize = localObj["logical_block_size"].toInt();
                    unsigned long long lbaWritten = attrObj["raw"].toObject()["value"].toVariant().toLongLong();
                    unsigned long long oneGB = static_cast<unsigned long long>(std::pow(2, 30));
                    unsigned long long totalGbWritten = (lbaWritten * logicalBlockSize) / oneGB;
                    totalWrites = QString::number(static_cast<int>(totalGbWritten)) + " GB";
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
            }
        }
    }

    totalReadsLineEdit->setText(totalReads);
    totalReadsLineEdit->setAlignment(Qt::AlignRight);

    totalWritesLineEdit->setText(totalWrites);
    totalWritesLineEdit->setAlignment(Qt::AlignRight);

    if (temperatureInt > 55) {
        temperatureValue->setStyleSheet("background-color: " + QColor(Qt::red).name() + ";");
    } else if ((temperatureInt < 55) && (temperatureInt > 50)){
        temperatureValue->setStyleSheet("background-color: " + QColor(Qt::yellow).name() + ";");
    } else if (temperatureInt == 0) {
        temperatureValue->setStyleSheet("background-color: " + QColor(Qt::gray).name() + ";");
    } else {
        temperatureValue->setStyleSheet("background-color: " + QColor(Qt::green).name() + ";");
    }

    QString labelStyle = "font-size:12pt; font-weight:700; color:black";

    if (temperatureInt > 0) {
        temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + QString::number(temperatureInt) + " °C</span></p></body></html>");
    } else {
        temperatureValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>N/A</span></p></body></html>");
    }

    temperatureValue->setAlignment(Qt::AlignCenter);

    if (health == "Bad") {
        healthStatusValue->setStyleSheet("background-color: " + QColor(Qt::red).name() + ";");
    } else if (health == "Caution"){
        healthStatusValue->setStyleSheet("background-color: " + QColor(Qt::yellow).name() + ";");
    } else {
        healthStatusValue->setStyleSheet("background-color: " + QColor(Qt::green).name() + ";");
    }

    healthStatusValue->setText("<html><head/><body><p><span style='" + labelStyle +"'>" + health + "</span></p></body></html>");
    healthStatusValue->setAlignment(Qt::AlignCenter);

    if (protocol != "NVMe") {
        addSmartAttributesTable(attributes);
    } else {
        addNvmeLogTable(nvmeLog);
    }
}

void MainWindow::addNvmeLogTable(const QJsonObject &nvmeLog)
{
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels({"", "ID", "Attribute Name", "Raw Values"});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->setItemDelegateForColumn(0, new StatusDot(tableWidget));
    tableWidget->setRowCount(nvmeLog.size());

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
    for (auto smartItem = nvmeLog.constBegin(); smartItem != nvmeLog.constEnd(); ++smartItem) {
        QString id = QString("%1").arg(row, 2, 16, QChar('0')).toUpper();

        QString name = smartItem.key().replace("_", " ");
        name = toTitleCase(name);

        QString raw = QString::number(smartItem.value().toInt());
        raw = QString("%1").arg(raw.toUInt(nullptr), 14, 16, QChar('0')).toUpper();


        QColor statusColor;
        statusColor = Qt::green; // For now leave it all green

        QTableWidgetItem *statusItem = new QTableWidgetItem();
        statusItem->setBackground(Qt::transparent);
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
        raw = QString("%1").arg(raw.toUInt(nullptr), 12, 16, QChar('0')).toUpper();

        QColor statusColor;
        if ((thresh != 0) && (value < thresh)) {
            statusColor = Qt::red;
        } else if ((id == "05" || id == "C5" || id == "C6") && (raw != "000000000000")) {
            statusColor = Qt::yellow;
        } else {
            statusColor = Qt::green;
        }

        QTableWidgetItem *statusItem = new QTableWidgetItem();
        statusItem->setBackground(Qt::transparent);
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
