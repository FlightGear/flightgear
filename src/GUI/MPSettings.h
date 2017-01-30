#ifndef MP_SETTINGS_H
#define MP_SETTINGS_H

#include <GUI/settingssection.h>

namespace Ui {
class MPSettings;
}

class MPSettings : public SettingsSection
{
    Q_OBJECT

public:
    explicit MPSettings(QWidget *parent = 0);
    ~MPSettings();

private:
    Ui::MPSettings *ui;
};

#endif // MP_SETTINGS_H
