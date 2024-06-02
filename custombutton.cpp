#include "custombutton.h"
#include <QFontMetrics>

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

    QString lines[] = {text1, text2, text3};
    int lineHeight = height() / 3;

    for (int i = 0; i < 3; ++i) {
        QFont font = painter.font();
        int fontSize = 1;
        painter.setFont(font);

        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(rect, Qt::AlignLeft, lines[i]);

        while (textRect.width() > rect.width() || textRect.height() > lineHeight) {
            fontSize--;
            font.setPointSize(fontSize);
            painter.setFont(font);
            fm = QFontMetrics(font);
            textRect = fm.boundingRect(rect, Qt::AlignLeft, lines[i]);
        }

        int yOffset = lineHeight * i + (lineHeight - textRect.height()) / 3;
        painter.drawText(QRect(rect.left(), yOffset, rect.width(), textRect.height()), Qt::AlignLeft, lines[i]);
    }
}
