#ifndef SETTINGSSECTIONQML_HXX
#define SETTINGSSECTIONQML_HXX

#include "SettingsSection.hxx"

#include <QQmlListProperty>

class SettingsControl;

class SettingsSectionQML : public SettingsSection
{
    Q_OBJECT


    Q_PROPERTY(QQmlListProperty<QObject> controls READ qmlControls)
    Q_PROPERTY(QString summary READ summary WRITE setSummary NOTIFY qmlSummaryChanged)

    Q_CLASSINFO("DefaultProperty", "controls")
public:
    SettingsSectionQML();

    QQmlListProperty<QObject> qmlControls();

    QList<SettingsControl*> controls() const;

    void saveState(QSettings& settings) const override;

    void restoreState(QSettings& settings) override;

    void doApply() override;

    virtual QString summary() const override;

    Q_INVOKABLE void saveSetting(QString key, QVariant value);
    Q_INVOKABLE QVariant restoreSetting(QString key);

     bool showAdvanced() const override;
public slots:
    void setSummary(QString summary);

    void setSearchTerm(QString search);
signals:
    void controlsChanged();

    /**
     * @brief apply - change the launch configuration according to the values
     * in this settings section.
     */
    void apply();

    void save();

    void restore();

    void qmlSummaryChanged(QString summary);

private:

    static void controls_append( QQmlListProperty<QObject>* prop,
                                 QObject* item );
    static void controls_clear( QQmlListProperty<QObject>* prop );
    static int controls_count( QQmlListProperty<QObject>* prop );
    static QObject* control_at( QQmlListProperty<QObject>* prop, int index );

    void internalUpdateAdvanced() override;
    void updateShowAdvanced() override;
    QString m_summary;
    QObjectList m_controls;
    bool m_forceShowAdvanced; ///< overrides show-advanced when searching
};

#endif // SETTINGSSECTIONQML_HXX
