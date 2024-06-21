#pragma once

#include <QColor>
#include <QPushButton>
#include <QString>
#include <QWidget>

class CustomButton : public QPushButton {
    Q_OBJECT

public:
    CustomButton(const QString &text1, const QString &text2, const QString &text3, const QColor &lineColor, QWidget *parent = nullptr);

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
