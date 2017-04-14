#ifndef SETTINGSSECTION_H
#define SETTINGSSECTION_H

#include <QFrame>
#include <QLabel>

class AdvancedSettingsButton;
class QSettings;
class LaunchConfig;

class SettingsSection : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

    Q_PROPERTY(bool showAdvanced READ showAdvanced WRITE setShowAdvanced NOTIFY showAdvancedChanged)

public:
    SettingsSection(QWidget* pr = nullptr);

    virtual void setLaunchConfig(LaunchConfig* config);

    virtual bool showAdvanced() const
    {
        return m_showAdvanced;
    }

    QString title() const
    {
        return m_title;
    }

    void insertSettingsHeader();

    virtual void saveState(QSettings& settings) const;

    virtual void restoreState(QSettings& settings);

    virtual void doApply() = 0;

    virtual QString summary() const = 0;

public slots:
    void setShowAdvanced(bool showAdvanced);

    void setTitle(QString title);

    void toggleShowAdvanced();

signals:
    void showAdvancedChanged(bool showAdvanced);

    void titleChanged(QString title);

    void summaryChanged(QString summary);

protected:
    virtual void internalUpdateAdvanced();
    virtual void updateShowAdvanced();

    QString m_title;
    bool m_showAdvanced = false;

    QLabel* m_titleLabel;
    AdvancedSettingsButton* m_advancedModeToggle;
};

#endif // SETTINGSSECTION_H
