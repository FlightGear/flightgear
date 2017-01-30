#ifndef ADVANCEDSETTINGSBUTTON_H
#define ADVANCEDSETTINGSBUTTON_H

#include <QAbstractButton>

class AdvancedSettingsButton : public QAbstractButton
{
public:
    AdvancedSettingsButton();

protected:
    virtual void paintEvent(QPaintEvent* event) override;

    virtual QSize sizeHint() const override;
private:
    void updateUi();
};

#endif // ADVANCEDSETTINGSBUTTON_H
