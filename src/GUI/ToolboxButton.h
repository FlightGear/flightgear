#ifndef TOOLBOXBUTTON_H
#define TOOLBOXBUTTON_H

#include <QAbstractButton>

class ToolboxButton : public QAbstractButton
{
public:
    ToolboxButton(QWidget* pr = nullptr);

protected:
    virtual void paintEvent(QPaintEvent* event) override;

    virtual QSize sizeHint() const override;

private:
};

#endif // TOOLBOXBUTTON_H
