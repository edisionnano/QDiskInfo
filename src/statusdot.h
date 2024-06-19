#ifndef STATUSDOT_H
#define STATUSDOT_H

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

class StatusDot : public QStyledItemDelegate
{
public:
    StatusDot(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif
