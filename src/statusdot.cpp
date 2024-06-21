#include "statusdot.h"

void StatusDot::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto opt = option;
    initStyleOption(&opt, index);
    int dotSize = 15;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QRect dotRect(opt.rect.center().x() - dotSize / 2, opt.rect.center().y() - dotSize / 2, dotSize, dotSize);
    auto color = QColor(index.data(Qt::BackgroundRole).value<QColor>());
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(dotRect);
    painter->restore();
}
