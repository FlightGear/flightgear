#ifndef FGCANVASWIDGET_H
#define FGCANVASWIDGET_H

#include <QWidget>
#include "localprop.h"

class FGCanvasGroup;

class FGCanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FGCanvasWidget(QWidget *parent = 0);

    void setRootProperty(LocalProp *root);

    FGCanvasGroup* rootElement();
signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent* pe);

private:
    LocalProp* _canvasRoot = nullptr;
    FGCanvasGroup* _rootElement = nullptr;
};

#endif // FGCANVASWIDGET_H
