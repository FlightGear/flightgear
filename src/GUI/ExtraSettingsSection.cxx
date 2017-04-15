#include "ExtraSettingsSection.hxx"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSettings>

#include "LauncherArgumentTokenizer.hxx"
#include "LaunchConfig.hxx"
#include "AdvancedSettingsButton.h"

ExtraSettingsSection::ExtraSettingsSection(QWidget* pr) :
    SettingsSection(pr)
{
    setTitle(tr("Additional Settings"));

    QVBoxLayout* topLevelVBox = qobject_cast<QVBoxLayout*>(layout());
    QLabel* prompt = new QLabel(this);
    prompt->setText("Enter additional command-line arguments if any are required. "
                    "See <a href=\"http://flightgear.sourceforge.net/getstart-en/getstart-enpa2.html#x5-450004.5\">here</a> "
                    "for documentation on possible arguments.");
    prompt->setWordWrap(true);
    prompt->setOpenExternalLinks(true);

    topLevelVBox->addWidget(prompt);
    m_argsEdit = new QTextEdit(this);
    m_argsEdit->setAcceptRichText(false);

#if QT_VERSION >= 0x050300
    // don't require Qt 5.3
    m_argsEdit->setPlaceholderText("--option=value --prop:/sim/name=value");
#endif
    topLevelVBox->addWidget(m_argsEdit);

    insertSettingsHeader();
    setShowAdvanced(false);
    m_advancedModeToggle->setVisible(false);
}

QString ExtraSettingsSection::argsText() const
{
    return m_argsEdit->toPlainText();
}

void ExtraSettingsSection::setLaunchConfig(LaunchConfig* config)
{
    m_config = config;
    SettingsSection::setLaunchConfig(config);
}

void ExtraSettingsSection::doApply()
{
    LauncherArgumentTokenizer tk;

    Q_FOREACH(auto arg, tk.tokenize(m_argsEdit->toPlainText())) {
        if (arg.arg.startsWith("prop:")) {
            m_config->setProperty(arg.arg.mid(5), arg.value);
        } else {
            m_config->setArg(arg.arg, arg.value);
        }
    }
}

void ExtraSettingsSection::saveState(QSettings &settings) const
{
    settings.setValue("extra-args", m_argsEdit->toPlainText());
}

void ExtraSettingsSection::restoreState(QSettings &settings)
{
    m_argsEdit->setText(settings.value("extra-args").toString());
}

QString ExtraSettingsSection::summary() const
{
    return QString();
}

void ExtraSettingsSection::validateText()
{

}

