#ifndef EXTRASETTINGSSECTION_HXX
#define EXTRASETTINGSSECTION_HXX

#include "settingssection.h"

class QTextEdit;
class LaunchConfig;

class ExtraSettingsSection : public SettingsSection
{
    Q_OBJECT
public:
    ExtraSettingsSection(QWidget* pr = nullptr);

    QString argsText() const;

    void setLaunchConfig(LaunchConfig* config) override;

    virtual void doApply() override;

    void saveState(QSettings &settings) const override;

    void restoreState(QSettings &settings) override;

    QString summary() const;

protected:

private:
    void validateText();

    QTextEdit* m_argsEdit;
    LaunchConfig* m_config;
};

#endif // EXTRASETTINGSSECTION_HXX
