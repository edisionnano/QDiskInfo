#pragma once

#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>

class GridView : public QWidget {
    Q_OBJECT

public:
    explicit GridView(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    struct DiskItem {
        QString name;
        QString category;
        QString icon;
    };

    QString searchQuery;
    QScrollArea *scrollArea;
    QWidget *gridContainer;
    QGridLayout *gridLayout;
    QPushButton *selectedButton;
    QString bgColor, borderColor, hoverColor, selectedColor;

    QList<DiskItem> disks;

    void extracted(const QVector<DiskItem> &filteredApps, int &cols, int &row, int &col);
    void populateGrid();
};
