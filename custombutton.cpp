#include "custombutton.h"

CustomButton::CustomButton(const QString &text1, const QString &text2, const QString &text3, const QColor &lineColor, QWidget *parent)
    : QPushButton(parent), text1(text1), text2(text2), text3(text3), lineColor(lineColor) {
    setMinimumHeight(60);
    setMinimumWidth(100);
}

void CustomButton::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);
    QPainter painter(this);
    QPalette pal = palette();
    QColor textColor = pal.color(QPalette::ButtonText);

    QPen pen(lineColor, 5);
    painter.setPen(pen);
    painter.drawLine(10, 9, 10, 50);

    QRect rect(20, 0, width() - 25, height());
    painter.setPen(textColor);
    painter.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, text1 + "\n" + text2 + "\n" + text3);
}
