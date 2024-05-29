#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>
#include <QPainter>

class CustomButton : public QPushButton {
    Q_OBJECT

public:
    CustomButton(const QString &text1, const QString &text2, const QString &text3, const QColor &lineColor, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString text1;
    QString text2;
    QString text3;
    QColor lineColor;
};

#endif
