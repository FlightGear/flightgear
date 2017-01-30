#ifndef RENDERINGSETTINGS_H
#define RENDERINGSETTINGS_H

#include <GUI/settingssection.h>

namespace Ui {
class RenderingSettings;
}

class RenderingSettings : public SettingsSection
{
    Q_OBJECT

public:
    explicit RenderingSettings(QWidget *parent = 0);
    ~RenderingSettings();

private:
    Ui::RenderingSettings *ui;
};

#endif // RENDERINGSETTINGS_H
