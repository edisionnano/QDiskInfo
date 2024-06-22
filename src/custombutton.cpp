#include "custombutton.h"

#include <QFontMetrics>
#include <QPainter>

CustomButton::CustomButton(const QString &text1_, const QString &text2_, const QString &text3_, const QColor &lineColor_, QWidget *parent)
    : QPushButton(parent), text1(text1_), text2(text2_), text3(text3_), lineColor(lineColor_) {
    setMinimumHeight(60);
    adjustWidthToFitText();
}

void CustomButton::adjustWidthToFitText() {
    QFont font = this->font();
    QFontMetrics fm(font);

    int maxWidth = 0;
    QString lines[] = {text1, text2, text3};

    for (const QString &line : lines) {
        int lineWidth = fm.horizontalAdvance(line);
        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    int desiredWidth = maxWidth + 28;
    setMinimumWidth(desiredWidth);
    setMaximumWidth(desiredWidth);
}

void CustomButton::paintEvent(QPaintEvent *event) {
    QPushButton::paintEvent(event);
    QPainter painter(this);
    QPalette pal = palette();
    QColor textColor = pal.color(QPalette::ButtonText);

    QPen pen(lineColor, 6);
    painter.setPen(pen);
    painter.drawLine(11, 10, 11, 50);

    QRect rect(20, 0, width() - 25, height());
    painter.setPen(textColor);
    painter.drawText(rect, Qt::AlignVCenter | Qt::AlignLeft, text1 + "\n" + text2 + "\n" + text3);
}

void CustomButton::resizeEvent(QResizeEvent *event) {
    QPushButton::resizeEvent(event);
    adjustWidthToFitText();
}

void CustomButton::setText1(const QString &newText1) {
    if (text1 != newText1) {
        text1 = newText1;
        adjustWidthToFitText();
        update();
    }
}

void CustomButton::setText2(const QString &newText2) {
    if (text2 != newText2) {
        text2 = newText2;
        adjustWidthToFitText();
        update();
    }
}

void CustomButton::setText3(const QString &newText3) {
    if (text3 != newText3) {
        text3 = newText3;
        adjustWidthToFitText();
        update();
    }
}
