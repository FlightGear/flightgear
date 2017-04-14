#include "SettingsSectionQML.hxx"

#include <QVBoxLayout>
#include <QSettings>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDebug>

#include "AdvancedSettingsButton.h"
#include "SettingsWidgets.hxx"
#include "LaunchConfig.hxx"

SettingsSectionQML::SettingsSectionQML()
{

}

void SettingsSectionQML::internalUpdateAdvanced()
{
    const bool showAdvanced = m_showAdvanced | m_forceShowAdvanced;
    Q_FOREACH (SettingsControl* w, controls()) {
        w->updateWidgetVisibility(showAdvanced);
    }
}

void SettingsSectionQML::controls_append(QQmlListProperty<QObject> *prop, QObject *item)
{
    SettingsSectionQML* self = qobject_cast<SettingsSectionQML*>(prop->object);
    QVBoxLayout* topLevelVBox = qobject_cast<QVBoxLayout*>(self->layout());
    self->m_controls.append(item);

    SettingsControl* control = qobject_cast<SettingsControl*>(item);
    if (control) {
        // following two lines would not be needed if the custom
        // setParent function was working :(
        control->setParent(nullptr);
        control->setParent(self);
        topLevelVBox->addWidget(control);
    }
}

void SettingsSectionQML::controls_clear(QQmlListProperty<QObject> *prop)
{
    SettingsSectionQML* self = qobject_cast<SettingsSectionQML*>(prop->object);
    QVBoxLayout* topLevelVBox = qobject_cast<QVBoxLayout*>(self->layout());

    Q_FOREACH (QObject* c, self->m_controls) {
        SettingsControl* control = qobject_cast<SettingsControl*>(c);
        if (control) {
            topLevelVBox->removeWidget(control);
        }
    }

}

int SettingsSectionQML::controls_count(QQmlListProperty<QObject> *prop)
{
    SettingsSectionQML* self = qobject_cast<SettingsSectionQML*>(prop->object);
    return self->m_controls.count();
}

QObject *SettingsSectionQML::control_at(QQmlListProperty<QObject> *prop, int index)
{
    SettingsSectionQML* self = qobject_cast<SettingsSectionQML*>(prop->object);
    return self->m_controls.at(index);
}

QQmlListProperty<QObject> SettingsSectionQML::qmlControls()
{
    return QQmlListProperty<QObject>(this, nullptr,
                                     &SettingsSectionQML::controls_append,
                                     &SettingsSectionQML::controls_count,
                                     &SettingsSectionQML::control_at,
                                     &SettingsSectionQML::controls_clear);
}

QList<SettingsControl *> SettingsSectionQML::controls() const
{
    return findChildren<SettingsControl*>();
}

void SettingsSectionQML::updateShowAdvanced()
{
    bool needsShowAdvanced = false;
    Q_FOREACH (SettingsControl* w, controls()) {
        needsShowAdvanced |= w->advanced();
    }

    m_advancedModeToggle->setVisible(needsShowAdvanced);
}

void SettingsSectionQML::saveState(QSettings &settings) const
{
    QQmlContext* context = QQmlEngine::contextForObject(this);
    QString s = context->nameForObject(const_cast<SettingsSectionQML*>(this));
    settings.beginGroup(s);
    Q_FOREACH (SettingsControl* control, controls()) {
        control->saveState(settings);
    }
    const_cast<SettingsSectionQML*>(this)->save();
    settings.endGroup();
}

void SettingsSectionQML::restoreState(QSettings &settings)
{
    QQmlContext* context = QQmlEngine::contextForObject(this);
    QString s = context->nameForObject(const_cast<SettingsSectionQML*>(this));
    settings.beginGroup(s);
    Q_FOREACH (SettingsControl* control, controls()) {
        control->restoreState(settings);
    }
    settings.endGroup();
    emit restore();
}

void SettingsSectionQML::doApply()
{
    LaunchConfig* config = qobject_cast<LaunchConfig*>(sender());
    Q_FOREACH (SettingsControl* control, controls()) {
        control->apply(config);
    }
    emit apply();
}

QString SettingsSectionQML::summary() const
{
    return m_summary;
}

void SettingsSectionQML::saveSetting(QString key, QVariant value)
{
    QSettings settings;
    QQmlContext* context = QQmlEngine::contextForObject(this);
    QString s = context->nameForObject(const_cast<SettingsSectionQML*>(this));
    settings.beginGroup(s);
    settings.setValue(key, value);
}

QVariant SettingsSectionQML::restoreSetting(QString key)
{
    QSettings settings;
    QQmlContext* context = QQmlEngine::contextForObject(this);
    QString s = context->nameForObject(const_cast<SettingsSectionQML*>(this));
    settings.beginGroup(s);
    return settings.value(key);
}

bool SettingsSectionQML::showAdvanced() const
{
    return m_showAdvanced | m_forceShowAdvanced;
}

void SettingsSectionQML::setSummary(QString summary)
{
    if (m_summary == summary)
        return;

    m_summary = summary;
    emit qmlSummaryChanged(summary);
    emit summaryChanged(summary);
}

void SettingsSectionQML::setSearchTerm(QString search)
{
    m_forceShowAdvanced = false;
    Q_FOREACH(SettingsControl* control, controls()) {
        const bool matches = control->setSearchTerm(search);
        m_forceShowAdvanced |= (matches && control->advanced());
    }
    internalUpdateAdvanced();
}
