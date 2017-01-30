#ifndef VIEWSETTINGS_H
#define VIEWSETTINGS_H

#include <GUI/settingssection.h>

namespace Ui {
class ViewSettings;
}

class ViewSettings : public SettingsSection
{
    Q_OBJECT

public:
    explicit ViewSettings(QWidget *parent = 0);
    ~ViewSettings();

private:
    Ui::ViewSettings *ui;
};

#endif // VIEWSETTINGS_H
