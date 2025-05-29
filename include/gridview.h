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

    void extracted(const QVector<DiskItem> &filteredDisks, int &cols, int &row, int &col);
    void populateGrid();
};
