#include "gridview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QIcon>
#include <QCheckBox>
#include <QComboBox>
#include <QPalette>
#include <QLineEdit>

int gridLayoutSpacing = 10;
int iconButtonSize = 64;

GridView::GridView(QWidget *parent) : QWidget(parent) {
    setWindowTitle(tr("Grid View"));
    resize(600, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QComboBox *searchField = new QComboBox();
    searchField->setEditable(true);
    QLineEdit *lineEdit = searchField->lineEdit();
    lineEdit->setPlaceholderText(tr("Search for a disk..."));
    connect(lineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        this->searchQuery = text;
        populateGrid();
    });
    mainLayout->addWidget(searchField);

    QPalette palette = this->palette();
    bgColor = palette.color(QPalette::Base).name();
    borderColor = palette.color(QPalette::Mid).name();
    hoverColor = palette.color(QPalette::Highlight).name();
    selectedColor = palette.color(QPalette::Highlight).name();

    QFrame *gridFrame = new QFrame();

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(gridFrame);
    mainLayout->addWidget(scrollArea);

    gridContainer = new QWidget();
    gridLayout = new QGridLayout(gridContainer);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    gridLayout->setSpacing(gridLayoutSpacing);
    gridFrame->setLayout(gridLayout);

    setLayout(mainLayout);
}

void GridView::resizeEvent(QResizeEvent *) {
    populateGrid();
}

void GridView::setDisks(const QVector<DiskItem> &newDisks) {
    disks = newDisks;
    populateGrid();
}

void GridView::highlightDisk(qsizetype index) {
    if (index < 0 || index >= gridLayout->count())
        return;

    QWidget *diskWidget = gridLayout->itemAt(static_cast<int>(index))->widget();
    if (!diskWidget)
        return;

    QPushButton *iconButton = diskWidget->findChild<QPushButton *>();
    if (!iconButton)
        return;

    if (selectedButton) {
        selectedButton->setStyleSheet(iconButton->styleSheet());
    }

    selectedButton = iconButton;
    selectedButton->setStyleSheet(
        QString("QPushButton { border: 2px solid %1; background-color: %2; }")
            .arg(selectedColor, hoverColor));
}

void GridView::setActiveIndex(qsizetype index) {
    activeIndex = index;
    if (gridLayout && gridLayout->count() > 0) {
        highlightDisk(activeIndex);
    }
}

void GridView::populateGrid() {
    selectedButton = nullptr;
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    int width = scrollArea->viewport()->width();
    int columnsFormula = (width + gridLayoutSpacing) / (iconButtonSize + gridLayoutSpacing);
    int cols = qMax(1, columnsFormula - 1);
    int row = 0, col = 0;


    QVector<DiskItem> filteredDisks;
    filteredDisks.reserve(disks.size());

    for (auto it = disks.cbegin(); it != disks.cend(); ++it) {
        const DiskItem &disk = *it;
        if (searchQuery.isEmpty() ||
            disk.name.contains(searchQuery, Qt::CaseInsensitive) ||
            disk.temperature.contains(searchQuery, Qt::CaseInsensitive)) {
            filteredDisks.append(disk);
        }
    }

    extracted(filteredDisks, cols, row, col);

    if (activeIndex >= 0 && activeIndex < filteredDisks.size()) {
        highlightDisk(activeIndex);
    }
}

void GridView::extracted(const QVector<DiskItem> &filteredDisks, int &cols, int &row, int &col) {
    for (int i = 0; i < filteredDisks.size(); ++i) {
        const auto &disk = filteredDisks[i];
        QWidget *diskWidget = new QWidget();
        QVBoxLayout *diskLayout = new QVBoxLayout(diskWidget);
        diskLayout->setAlignment(Qt::AlignCenter);
        diskLayout->setContentsMargins(0, 0, 0, 0);

        QPushButton *iconButton = new QPushButton();
        QString iconPath = QString(":/icons/Disk_%1.svg").arg(disk.health);
        iconButton->setIcon(QIcon(iconPath));
        iconButton->setIconSize(QSize(48, 48));
        iconButton->setFixedSize(iconButtonSize, iconButtonSize);
        iconButton->setStyleSheet(
            QString("QPushButton { border: 2px solid transparent; "
                    "border-radius: 10px; }"
                    "QPushButton:hover { background-color: %1; }"
                    "QPushButton:pressed { border: 2px solid %2; }")
                .arg(hoverColor, selectedColor));

        QLabel *nameLabel = new QLabel(disk.name);
        nameLabel->setAlignment(Qt::AlignCenter);
        QLabel *temperatureLabel = new QLabel(disk.temperature);
        temperatureLabel->setAlignment(Qt::AlignCenter);
        temperatureLabel->setStyleSheet("font-size: 10px; color: gray;");

        connect(iconButton, &QPushButton::clicked, this, [this, iconButton, disk, i]() {
            if (selectedButton) {
                selectedButton->setStyleSheet(iconButton->styleSheet());
            }
            selectedButton = iconButton;
            selectedButton->setStyleSheet(
                QString(
                    "QPushButton { border: 2px solid %1; background-color: %2; }")
                    .arg(selectedColor, hoverColor));
            emit diskSelected(i);
        });

        diskLayout->addWidget(iconButton, 0, Qt::AlignCenter);
        diskLayout->addWidget(nameLabel, 0, Qt::AlignCenter);
        diskLayout->addWidget(temperatureLabel, 0, Qt::AlignCenter);
        diskWidget->setLayout(diskLayout);
        gridLayout->addWidget(diskWidget, row, col);

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }
}
