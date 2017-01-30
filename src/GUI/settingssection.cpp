#include "settingssection.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QVariant>

#include "AdvancedSettingsButton.h"

SettingsSection::SettingsSection(QWidget* pr) :
    QFrame(pr)
{
    m_titleLabel = new QLabel;
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    QPalette pal = palette();
    pal.setColor(QPalette::Normal, QPalette::WindowText, Qt::white);
    m_titleLabel->setPalette(pal);
}

void SettingsSection::setShowAdvanced(bool showAdvanced)
{
    if (m_showAdvanced == showAdvanced)
        return;

    m_showAdvanced = showAdvanced;
    internalUpdateAdvanced();
    emit showAdvancedChanged(showAdvanced);
}

void SettingsSection::setTitle(QString title)
{
    if (m_title == title)
        return;

    m_title = title;
    m_titleLabel->setText(m_title);
    emit titleChanged(title);
}

void SettingsSection::toggleShowAdvanced()
{
    setShowAdvanced(!m_showAdvanced);
}

void SettingsSection::insertSettingsHeader()
{
    QVBoxLayout* topLevelVBox = qobject_cast<QVBoxLayout*>(layout());
    Q_ASSERT(topLevelVBox);

    topLevelVBox->setMargin(0);

    QFrame* headerPanel = new QFrame(this);
    headerPanel->setFrameStyle(QFrame::Box);
    headerPanel->setAutoFillBackground(true);

    QPalette p = headerPanel->palette();
    p.setColor(QPalette::Normal, QPalette::Background, QColor(0x7f, 0x7f, 0x7f));
    p.setColor(QPalette::Normal, QPalette::Foreground, Qt::black);
    p.setColor(QPalette::Normal, QPalette::WindowText, Qt::white);
    headerPanel->setPalette(p);

    topLevelVBox->insertWidget(0, headerPanel);

    QHBoxLayout* hbox = new QHBoxLayout(headerPanel);
    hbox->setContentsMargins(32, 0, 32, 0);
    headerPanel->setLayout(hbox);

    hbox->addWidget(m_titleLabel);
    hbox->addStretch(1);

    m_advancedModeToggle = new AdvancedSettingsButton;
    connect(m_advancedModeToggle, &QPushButton::toggled, this, &SettingsSection::toggleShowAdvanced);

    hbox->addWidget(m_advancedModeToggle);

    QFont helpLabelFont;
    helpLabelFont.setPointSize(helpLabelFont.pointSize() - 1);

    QPalette pal = palette();
    pal.setColor(QPalette::Normal, QPalette::WindowText, QColor(0x3f, 0x3f, 0x3f));

    Q_FOREACH(QLabel* w,  findChildren<QLabel*>()) {
        if (w->property("help").toBool()) {
            w->setFont(helpLabelFont);
            w->setPalette(pal);
        }
    }

    internalUpdateAdvanced();
}

void SettingsSection::internalUpdateAdvanced()
{
    Q_FOREACH(QWidget* w,  findChildren<QWidget*>()) {
        if (w->property("advanced").toBool()) {
            w->setVisible(m_showAdvanced);
        }
    }
}
