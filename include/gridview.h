#pragma once

#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>

#include "diskitem.h"

class GridView : public QWidget {
    Q_OBJECT

public:
    explicit GridView(QWidget *parent = nullptr);
    void setDisks(const QVector<DiskItem> &newDisks);
    void highlightDisk(qsizetype index);
    void setActiveIndex(qsizetype index);

protected:
    void resizeEvent(QResizeEvent *) override;

signals:
    void diskSelected(int index);

private:
    QString searchQuery;
    QScrollArea *scrollArea;
    QWidget *gridContainer;
    QGridLayout *gridLayout;
    QPushButton *selectedButton;
    QString bgColor, borderColor, hoverColor, selectedColor;

    QList<DiskItem> disks;

    void populateGrid();
    void extractDisksFromVector(const QVector<DiskItem> &filteredDisks, int &cols, int &row, int &col);

    qsizetype activeIndex = -1;
};
