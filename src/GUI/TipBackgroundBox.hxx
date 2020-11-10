#pragma once

#include <QQuickPaintedItem>
#include <QColor>

#include <GUI/GettingStartedTip.hxx>

class TipBackgroundBox : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(GettingStartedTip::Arrow arrow READ arrowPosition WRITE setArrowPosition NOTIFY arrowPositionChanged)

    Q_PROPERTY(QColor fill READ fill WRITE setFill NOTIFY fillChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)

public:
    TipBackgroundBox(QQuickItem* parent = nullptr);

    GettingStartedTip::Arrow arrowPosition() const;

    QColor borderColor() const;

    int borderWidth() const;

    QColor fill() const;

    static int arrowSideOffset();

    static int arrowHeight();

public slots:
    void setArrowPosition(GettingStartedTip::Arrow arrow);

    void setBorderColor(QColor borderColor);

    void setBorderWidth(int borderWidth);

    void setFill(QColor fill);

signals:
    void arrowPositionChanged(GettingStartedTip::Arrow arrow);

    void borderColorChanged(QColor borderColor);

    void borderWidthChanged(int borderWidth);

    void fillChanged(QColor fill);

protected:
    void paint(QPainter* painter) override;

private:
    GettingStartedTip::Arrow _arrow = GettingStartedTip::Arrow::TopCenter;

    QColor _borderColor;
    int _borderWidth = -1; // off
    QColor _fill;
};

