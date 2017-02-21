#include "ViewCommandLinePage.hxx"

#include <QTextEdit>
#include <QVBoxLayout>

#include <Main/options.hxx>

#include "LaunchConfig.hxx"

#if 0
#include "ExtraSettingsSection.hxx"
#include "LauncherArgumentTokenizer.hxx"
#endif

ViewCommandLinePage::ViewCommandLinePage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    setLayout(vbox);
    m_browser = new QTextEdit(this);
    m_browser->setReadOnly(true);
    vbox->addWidget(m_browser);
}

#if 0
void ViewCommandLinePage::setExtraSettingsSection(ExtraSettingsSection *ess)
{
    m_extraSettings = ess;
}
#endif

void ViewCommandLinePage::setLaunchConfig(LaunchConfig *config)
{
    m_config = config;
}

void ViewCommandLinePage::update()
{
    QString html;
    string_list commandLineOpts = flightgear::Options::sharedInstance()->extractOptions();
    if (!commandLineOpts.empty()) {
        html += "<p>Options passed on the command line:</p>\n";
        html += "<ul>\n";
        for (auto opt : commandLineOpts) {
            html += QString("<li>--") + QString::fromStdString(opt) + "</li>\n";
        }
        html += "</ul>\n";
    }
#if 0
    if (m_extraSettings) {
        LauncherArgumentTokenizer tk;
        Q_FOREACH(auto arg, tk.tokenize(m_extraSettings->argsText())) {
    //        m_config->setArg(arg.arg, arg.value);
        }
    }
#endif
    m_config->reset();
    m_config->collect();

    html += "<p>Options set in the launcher:</p>\n";
    html += "<ul>\n";
    for (auto arg : m_config->values()) {
        if (arg.value.isEmpty()) {
            html += QString("<li>--") + arg.arg + "</li>\n";
        } else if (arg.arg == "prop") {
            html += QString("<li>--") + arg.arg + ":" + arg.value + "</li>\n";
        } else {
            html += QString("<li>--") + arg.arg + "=" + arg.value + "</li>\n";
        }
    }
    html += "</ul>\n";

    m_browser->setHtml(html);
}
