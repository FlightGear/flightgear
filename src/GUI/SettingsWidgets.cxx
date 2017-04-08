#include "SettingsWidgets.hxx"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QSettings>
#include <QLineEdit>
#include <QDebug>
#include <QPushButton>
#include <QFileDialog>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDateTimeEdit>
#include <QPainter>

#include "LaunchConfig.hxx"

const int MARGIN_HINT = 4;

/////////////////////////////////////////////////////////////////////////////////

QString SettingsControl::label() const
{
    return QString();
}

QString SettingsControl::description() const
{
    if (m_description) {
        return m_description->text();
    }

    return QString();
}

QStringList SettingsControl::keywords() const
{
    return m_keywords;
}

QString SettingsControl::option() const
{
    return m_option;
}

bool SettingsControl::setSearchTerm(QString search)
{
    bool inSearch = false;
    // only show matches when search string is at least three characters,
    // to avoid many hits when the user starts typing
    if (search.length() > 2) {
        Q_FOREACH(QString k, m_keywords) {
            if (k.indexOf(search, 0, Qt::CaseInsensitive) >= 0) {
                inSearch = true;
            }
        }
    }

    setProperty("search-result", inSearch);
    update();
    return inSearch;
}

void SettingsControl::setAdvanced(bool advanced)
{
    if (m_advanced == advanced)
        return;

    m_advanced = advanced;
    emit advancedChanged(advanced);
}

void SettingsControl::setDescription(QString desc)
{
    if (m_description) {
        m_description->setText(desc);
    }
    emit descriptionChanged();
}

void SettingsControl::apply(LaunchConfig *lconfig) const
{
    Q_UNUSED(lconfig)
}

void SettingsControl::setKeywords(QStringList keywords)
{
    if (m_keywords == keywords)
        return;

    m_keywords = keywords;
    emit keywordsChanged(keywords);
}

void SettingsControl::setOption(QString option)
{
    if (m_option == option)
        return;

    m_option = option;
    emit optionChanged(option);
}

SettingsControl::SettingsControl(QWidget *pr) :
    QWidget(pr)
{
}

void SettingsControl::createDescription()
{
    m_description = new QLabel(this);
    m_description->setWordWrap(true);
    QFont f = m_description->font();
    f.setPointSize(f.pointSize() - 2);
    m_description->setFont(f);
}

QString SettingsControl::qmlName() const
{
    QQmlContext* context = QQmlEngine::contextForObject(this);
    QString s = context->nameForObject(const_cast<SettingsControl*>(this));
    return s;
}

void SettingsControl::paintEvent(QPaintEvent *pe)
{
    QWidget::paintEvent(pe);

    if (property("search-result").toBool()) {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing);
        QPen pen(QColor("#7f7f7fff"), 1);
        painter.setPen(pen);
        painter.setBrush(QColor("#2f7f7fff"));
        painter.drawRoundedRect(rect(), 8, 8);
    }
}

/////////////////////////////////////////////////////////////////////////////////

SettingsCheckbox::SettingsCheckbox(QWidget* parent) :
    SettingsControl(parent)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(MARGIN_HINT);
    setLayout(vbox);
    m_check = new QCheckBox(this);
    vbox->addWidget(m_check);
    createDescription();
    vbox->addWidget(m_description);
    connect(m_check, &QCheckBox::toggled, this, &SettingsCheckbox::checkedChanged);
}

QString SettingsCheckbox::label() const
{
    return m_check->text();
}

bool SettingsCheckbox::isChecked() const
{
    return m_check->isChecked();
}

void SettingsCheckbox::setChecked(bool checked)
{
    if (checked == isChecked()) {
        return;
    }

    m_check->setChecked(checked);
    emit checkedChanged(checked);
}

void SettingsCheckbox::setLabel(QString label)
{
    m_check->setText(label);
}

void SettingsCheckbox::apply(LaunchConfig* lconfig) const
{
    if (option().isEmpty()) {
        return;
    }

    lconfig->setEnableDisableOption(option(), isChecked());
}

void SettingsCheckbox::saveState(QSettings &settings) const
{
    settings.setValue(qmlName(), isChecked());
}

void SettingsCheckbox::restoreState(QSettings &settings)
{
    setChecked(settings.value(qmlName(), isChecked()).toBool());
}

/////////////////////////////////////////////////////////////////////////////////

SettingsComboBox::SettingsComboBox(QWidget *pr) :
    SettingsControl(pr)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    setLayout(vbox);
    vbox->setMargin(MARGIN_HINT);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->setMargin(MARGIN_HINT);

    vbox->addLayout(hbox);
    m_combo = new QComboBox(this);

    m_label = new QLabel(this);
    hbox->addWidget(m_label);
    hbox->addWidget(m_combo);
    hbox->addStretch(1);

    createDescription();
    vbox->addWidget(m_description);

    connect(m_combo, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(selectedIndexChanged(int)));
}

QStringList SettingsComboBox::choices() const
{
    QStringList result;
    for (int i=0; i < m_combo->count(); ++i) {
        result.append(m_combo->itemText(i));
    }
    return result;
}

int SettingsComboBox::selectedIndex() const
{
    return m_combo->currentIndex();
}

void SettingsComboBox::saveState(QSettings &settings) const
{
    // if selected index is custom, need to save something else?
    if (selectedIndex() == m_defaultIndex) {
        settings.remove(qmlName());
    } else {
        settings.setValue(qmlName(), selectedIndex());
    }
}

void SettingsComboBox::restoreState(QSettings &settings)
{
    QString id = qmlName();
    setSelectedIndex(settings.value(id, m_defaultIndex).toInt());
}

void SettingsComboBox::setChoices(QStringList aChoices)
{
    if (choices() == aChoices)
        return;

    m_combo->clear();
    Q_FOREACH (QString choice, aChoices) {
        m_combo->addItem(choice);
    }

    emit choicesChanged(aChoices);
}

void SettingsComboBox::setSelectedIndex(int selectedIndex)
{
    if (m_combo->currentIndex() == selectedIndex)
        return;

    m_combo->setCurrentIndex(selectedIndex);
    emit selectedIndexChanged(selectedIndex);
}

void SettingsComboBox::setLabel(QString label)
{
    m_label->setText(label);
    emit labelChanged();
}


QAbstractItemModel *SettingsComboBox::model() const
{
    return m_combo->model();
}

int SettingsComboBox::defaultIndex() const
{
    return m_defaultIndex;
}

void SettingsComboBox::setModel(QAbstractItemModel *model)
{
    if (model == m_combo->model()) {
        return;
    }

    m_combo->setModel(model);
    emit modelChanged(model);
}

void SettingsComboBox::setDefaultIndex(int defaultIndex)
{
    if (m_defaultIndex == defaultIndex)
        return;

    m_defaultIndex = defaultIndex;
    emit defaultIndexChanged(defaultIndex);
}

/////////////////////////////////////////////////////////////////////////////////

SettingsIntSpinbox::SettingsIntSpinbox(QWidget *pr) :
    SettingsControl(pr)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(MARGIN_HINT);
    setLayout(vbox);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->setMargin(MARGIN_HINT);

    vbox->addLayout(hbox);
    m_spin = new QSpinBox(this);

    m_label = new QLabel(this);
    hbox->addWidget(m_label);
    hbox->addWidget(m_spin);
    hbox->addStretch(1);

    createDescription();
    vbox->addWidget(m_description);

    connect(m_spin, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
}

int SettingsIntSpinbox::value() const
{
    return m_spin->value();
}

int SettingsIntSpinbox::min() const
{
    return m_spin->minimum();
}

int SettingsIntSpinbox::max() const
{
    return m_spin->maximum();
}

void SettingsIntSpinbox::apply(LaunchConfig *lconfig) const
{
    if (option().isEmpty()) {
        return;
    }

    lconfig->setArg(option(), QString::number(value()));
}

void SettingsIntSpinbox::saveState(QSettings &settings) const
{
    settings.setValue(qmlName(), value());
}

void SettingsIntSpinbox::restoreState(QSettings &settings)
{
    setValue(settings.value(qmlName(), value()).toInt());
}

void SettingsIntSpinbox::setValue(int aValue)
{
    if (value() == aValue)
        return;

    m_spin->setValue(aValue);
    emit valueChanged(aValue);
}

void SettingsIntSpinbox::setMin(int aMin)
{
    if (min() == aMin)
        return;

    m_spin->setMinimum(aMin);
    emit minChanged(aMin);
}

void SettingsIntSpinbox::setMax(int aMax)
{
    if (max() == aMax)
        return;

    m_spin->setMaximum(aMax);
    emit maxChanged(aMax);
}

void SettingsIntSpinbox::setLabel(QString label)
{
    m_label->setText(label);
    emit labelChanged();
}

SettingsText::SettingsText(QWidget *pr) :
    SettingsControl(pr)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(MARGIN_HINT);
    setLayout(vbox);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->setMargin(MARGIN_HINT);
    vbox->addLayout(hbox);
    m_edit = new QLineEdit(this);

    m_label = new QLabel(this);
    hbox->addWidget(m_label);
    hbox->addWidget(m_edit, 1);
 //   hbox->addStretch(1);

    createDescription();
    vbox->addWidget(m_description);

    connect(m_edit, &QLineEdit::textChanged, this, &SettingsText::valueChanged);
}

QString SettingsText::value() const
{
    return m_edit->text();
}

void SettingsText::apply(LaunchConfig *lconfig) const
{
    if (option().isEmpty()) {
        return;
    }

    lconfig->setArg(option(), value());
}

void SettingsText::saveState(QSettings &settings) const
{
    settings.setValue(qmlName(), value());
}

void SettingsText::restoreState(QSettings &settings)
{
    setValue(settings.value(qmlName(), value()).toString());
}

void SettingsText::setLabel(QString label)
{
    m_label->setText(label);
}

void SettingsText::setValue(QString newVal)
{
    if (value() == newVal)
        return;

    m_edit->setText(newVal);
    emit valueChanged(newVal);
}

QString SettingsText::placeholder() const
{
#if QT_VERSION >= 0x050300
    return m_edit->placeholderText();
#else
    return QString();
#endif
}

void SettingsText::setPlaceholder(QString hold)
{
    if (placeholder() == hold)
        return;

#if QT_VERSION >= 0x050300
    // don't require Qt 5.3
    m_edit->setPlaceholderText(hold);
#endif
    emit placeholderChanged(hold);
}

SettingsPath::SettingsPath(QWidget *pr) :
    SettingsControl(pr)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(MARGIN_HINT);
    setLayout(vbox);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->setMargin(MARGIN_HINT);
    vbox->addLayout(hbox);

    m_changeButton = new QPushButton(tr("Change"), this);
    connect(m_changeButton, &QPushButton::clicked, this, &SettingsPath::choosePath);

    m_defaultButton = new QPushButton(tr("Default"), this);
    connect(m_defaultButton, &QPushButton::clicked, this, &SettingsPath::restoreDefaultPath);

    m_label = new QLabel(this);
    hbox->addWidget(m_label, 1);
    hbox->addWidget(m_changeButton);
    hbox->addWidget(m_defaultButton);

    createDescription();
    vbox->addWidget(m_description);
}

QString SettingsPath::path() const
{
    return m_path;
}

void SettingsPath::setPath(QString path)
{
    if (m_path == path)
        return;

    m_path = path;
    emit pathChanged(path);
    updateLabel();
}

void SettingsPath::setLabel(QString label)
{
    m_labelPrefix = label;
    updateLabel();
}

void SettingsPath::apply(LaunchConfig *lconfig) const
{
    if (m_option.isEmpty()) {
        return;
    }

    if (m_path.isEmpty()) {
        return;
    }

    lconfig->setArg(option(), path());
}

void SettingsPath::setDefaultPath(QString path)
{
    if (m_defaultPath == path)
        return;

    m_defaultPath = path;
    m_defaultButton->setVisible(!m_defaultPath.isEmpty());
    emit defaultPathChanged(path);
    updateLabel();
}

void SettingsPath::setChooseDirectory(bool chooseDirectory)
{
    if (m_chooseDirectory == chooseDirectory)
        return;

    m_chooseDirectory = chooseDirectory;
    emit chooseDirectoryChanged(chooseDirectory);
}

void SettingsPath::choosePath()
{
    QString path;
    if (m_chooseDirectory) {
        path = QFileDialog::getExistingDirectory(this,
                                                     m_dialogPrompt,
                                                     m_path);
    } else {
        // if we're going to use this, add filter support
        path = QFileDialog::getOpenFileName(this, m_dialogPrompt, m_path);
    }

    if (path.isEmpty()) {
        return; // user cancelled
    }
    setPath(path);
}

void SettingsPath::restoreDefaultPath()
{
    setPath(QString());
}

void SettingsPath::setDialogPrompt(QString dialogPrompt)
{
    if (m_dialogPrompt == dialogPrompt)
        return;

    m_dialogPrompt = dialogPrompt;
    emit dialogPromptChanged(dialogPrompt);
}

void SettingsPath::setOption(QString option)
{
    if (m_option == option)
        return;

    m_option = option;
    emit optionChanged(option);
}

void SettingsPath::updateLabel()
{
    const bool isDefault = (m_path.isEmpty());
    QString s = isDefault ? tr("%1: %2 (default)") : tr("%1: %2");
    QString path = isDefault ? defaultPath() : m_path;
    m_label->setText(s.arg(m_labelPrefix).arg(path));
    m_defaultButton->setEnabled(!isDefault);
}

void SettingsPath::saveState(QSettings &settings) const
{
    settings.setValue(qmlName(), m_path);
}

void SettingsPath::restoreState(QSettings &settings)
{
    QString s = settings.value(qmlName(), QString()).toString();
    setPath(s);
}

SettingsDateTime::SettingsDateTime(QWidget *pr) :
    SettingsControl(pr)
{
    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setMargin(MARGIN_HINT);
    setLayout(vbox);

    QHBoxLayout* hbox = new QHBoxLayout;
    hbox->setMargin(MARGIN_HINT);
    vbox->addLayout(hbox);

    m_edit = new QDateTimeEdit;
    m_label = new QLabel(this);
    hbox->addWidget(m_label);
    hbox->addWidget(m_edit);
    hbox->addStretch(1);

    createDescription();
    vbox->addWidget(m_description);
}

void SettingsDateTime::saveState(QSettings &settings) const
{
    settings.setValue(qmlName(), value());
}

void SettingsDateTime::restoreState(QSettings &settings)
{
    m_edit->setDateTime(settings.value(qmlName()).toDateTime());
}

void SettingsDateTime::setLabel(QString label)
{
    m_label->setText(label);
}

QDateTime SettingsDateTime::value() const
{
    return m_edit->dateTime();
}

void SettingsDateTime::setValue(QDateTime value)
{
    if (m_edit->dateTime() == value) {

    }

    emit valueChanged(value);
}

