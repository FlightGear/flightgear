#ifndef SETTINGSSECTION_H
#define SETTINGSSECTION_H

#include <QFrame>
#include <QLabel>

class AdvancedSettingsButton;

class SettingsSection : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)

    Q_PROPERTY(bool showAdvanced READ showAdvanced WRITE setShowAdvanced NOTIFY showAdvancedChanged)

public:
    SettingsSection(QWidget* pr = nullptr);

    bool showAdvanced() const
    {
        return m_showAdvanced;
    }

    QString title() const
    {
        return m_title;
    }

    void insertSettingsHeader();

public slots:
    void setShowAdvanced(bool showAdvanced);

    void setTitle(QString title);

    void toggleShowAdvanced();
signals:
    void showAdvancedChanged(bool showAdvanced);

    void titleChanged(QString title);

private:
    void internalUpdateAdvanced();

    QString m_title;
    bool m_showAdvanced = false;

    QLabel* m_titleLabel;
    AdvancedSettingsButton* m_advancedModeToggle;
};

#endif // SETTINGSSECTION_H
