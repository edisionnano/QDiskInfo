#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>
#include <QPainter>

class CustomButton : public QPushButton {
    Q_OBJECT

public:
    CustomButton(const QString &text1_, const QString &text2_, const QString &text3_, const QColor &lineColor_, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void setText1(const QString &newText1);
    void setText2(const QString &newText2);
    void setText3(const QString &newText3);
    void adjustWidthToFitText();

private:
    QString text1;
    QString text2;
    QString text3;
    QColor lineColor;
};

#endif
