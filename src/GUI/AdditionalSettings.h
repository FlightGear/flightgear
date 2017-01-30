#ifndef ADDITIONAL_SETTINGS_H
#define ADDITIONAL_SETTINGS_H

#include <GUI/settingssection.h>

namespace Ui {
class AdditionalSettings;
}

class AdditionalSettings : public SettingsSection
{
    Q_OBJECT

public:
    explicit AdditionalSettings(QWidget *parent = 0);
    ~AdditionalSettings();

private:
    Ui::AdditionalSettings *ui;
};

#endif // ADDITIONAL_SETTINGS_H
