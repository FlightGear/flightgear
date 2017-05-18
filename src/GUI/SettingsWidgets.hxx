#ifndef SETTINGSWIDGETS_HXX
#define SETTINGSWIDGETS_HXX

#include <QWidget>
#include <QDateTime>

class QCheckBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QLabel;
class QSettings;
class LaunchConfig;
class QPushButton;
class QAbstractItemModel;
class QDateTimeEdit;

class SettingsControl : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool advanced READ advanced WRITE setAdvanced NOTIFY advancedChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QStringList keywords READ keywords WRITE setKeywords NOTIFY keywordsChanged)
    Q_PROPERTY(QString option READ option WRITE setOption NOTIFY optionChanged)

    Q_PROPERTY(bool visible READ qmlVisible WRITE setQmlVisible NOTIFY qmlVisibleChanged)

public:

    bool advanced() const
    {
        return m_advanced;
    }

    virtual QString label() const;
    virtual QString description() const;

    QStringList keywords() const;

    QString option() const;

    virtual void saveState(QSettings& settings) const = 0;

    virtual void restoreState(QSettings& settings) = 0;

    /**
     * @brief setSearchTerm - update the search term and decide if this control
     * should e highlighted or not.
     * @param search - the text being searched
     * @return - if this control matched the search term or not
     */
    bool setSearchTerm(QString search);
    bool qmlVisible() const;

    void updateWidgetVisibility(bool showAdvanced);
public slots:
    void setAdvanced(bool advanced);

    virtual void setDescription(QString desc);
    virtual void setLabel(QString label) = 0;

    virtual void apply(LaunchConfig* lconfig) const;

    void setKeywords(QStringList keywords);

    void setOption(QString option);

    void setQmlVisible(bool visible);

signals:
    void advancedChanged(bool advanced);
    void labelChanged();
    void descriptionChanged();
    void keywordsChanged(QStringList keywords);
    void optionChanged(QString option);

    void qmlVisibleChanged(bool visible);

protected:
    SettingsControl(QWidget* pr = nullptr);

    void createDescription();

    QString qmlName() const;

    QLabel* m_description = nullptr;

    void paintEvent(QPaintEvent* pe) override;

    void updateWidgetVisibility();
private:
    bool m_advanced = false;
    QStringList m_keywords;
    QString m_option;
    bool m_localVisible = true;
};

class SettingsCheckbox : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged)

    bool isChecked() const;

public:
    SettingsCheckbox(QWidget* pr = nullptr);

    virtual QString label() const override;

    virtual void setLabel(QString label) override;

    virtual void apply(LaunchConfig* lconfig) const override;

    virtual void saveState(QSettings& settings) const override;

    virtual void restoreState(QSettings& settings) override;
public slots:
    void setChecked(bool checked);

signals:
    void checkedChanged(bool checked);

private:
    QCheckBox* m_check;
};

class SettingsComboBox : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QStringList choices READ choices WRITE setChoices NOTIFY choicesChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int defaultIndex READ defaultIndex WRITE setDefaultIndex NOTIFY defaultIndexChanged)
public:
    SettingsComboBox(QWidget* pr = nullptr);

    QStringList choices() const;

    int selectedIndex() const;

    virtual void saveState(QSettings& settings) const override;

    virtual void restoreState(QSettings& settings) override;

    QAbstractItemModel* model() const;

    int defaultIndex() const;

public slots:
    void setChoices(QStringList choices);

    void setSelectedIndex(int selectedIndex);

    void setModel(QAbstractItemModel* model);

    void setDefaultIndex(int defaultIndex);

signals:
    void choicesChanged(QStringList choices);

    void selectedIndexChanged(int selectedIndex);

    void modelChanged(QAbstractItemModel* model);

    void defaultIndexChanged(int defaultIndex);

protected:
    virtual void setLabel(QString label) override;

private:
    QLabel* m_label;
    QComboBox* m_combo;
    QStringList m_choices;
    int m_selectedIndex = 0;
    int m_defaultIndex = -1;
};

class SettingsIntSpinbox : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(int max READ max WRITE setMax NOTIFY maxChanged)

    // suffix / wrap information

public:
    SettingsIntSpinbox(QWidget* pr = nullptr);

    int value() const;
    int min() const;
    int max() const;

    virtual void apply(LaunchConfig* lconfig) const override;

    virtual void saveState(QSettings& settings) const override;

    virtual void restoreState(QSettings& settings) override;
public slots:
    void setValue(int value);
    void setMin(int min);
    void setMax(int max);

signals:
    void valueChanged(int value);
    void minChanged(int min);
    void maxChanged(int max);

protected:
    virtual void setLabel(QString label) override;

private:
    QSpinBox* m_spin;
    QLabel* m_label;
    int m_value;
    int m_min;
    int m_max;
};

class SettingsText : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString placeholder READ placeholder WRITE setPlaceholder NOTIFY placeholderChanged)

    Q_PROPERTY(QString validation READ validation WRITE setValidation NOTIFY validationChanged)

public:
    SettingsText(QWidget *pr = nullptr);

    QString value() const;

    virtual void apply(LaunchConfig* lconfig) const override;

    virtual void saveState(QSettings& settings) const override;

    virtual void restoreState(QSettings& settings) override;

    virtual void setLabel(QString label) override;

    QString placeholder() const;

    QString validation() const;

public slots:
    void setValue(QString value);

    void setPlaceholder(QString placeholder);

    void setValidation(QString validation);

signals:
    void valueChanged(QString value);

    void placeholderChanged(QString placeholder);

    void validationChanged(QString validation);

private:
    QLineEdit* m_edit;
    QLabel* m_label;
    QString m_validation;
};

class SettingsPath : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString defaultPath READ defaultPath WRITE setDefaultPath
                NOTIFY defaultPathChanged)
    Q_PROPERTY(bool chooseDirectory READ chooseDirectory
               WRITE setChooseDirectory NOTIFY chooseDirectoryChanged)
    Q_PROPERTY(QString dialogPrompt READ dialogPrompt
               WRITE setDialogPrompt NOTIFY dialogPromptChanged)
    Q_PROPERTY(QString option READ option WRITE setOption NOTIFY optionChanged)


public:
    SettingsPath(QWidget *pr = nullptr);

    QString path() const;

    virtual void saveState(QSettings& settings) const override;

    virtual void restoreState(QSettings& settings) override;

    virtual void setLabel(QString label) override;

    virtual void apply(LaunchConfig* lconfig) const override;

    QString defaultPath() const
    {
        return m_defaultPath;
    }

    bool chooseDirectory() const
    {
        return m_chooseDirectory;
    }

    QString dialogPrompt() const
    {
        return m_dialogPrompt;
    }

    QString option() const
    {
        return m_option;
    }

public slots:
    void setPath(QString path);
    void setDefaultPath(QString path);

    void setChooseDirectory(bool chooseDirectory);

    void choosePath();
    void restoreDefaultPath();
    void setDialogPrompt(QString dialogPrompt);

    void setOption(QString option);

signals:
    void defaultPathChanged(QString defaultPath);

    void pathChanged(QString path);

    void chooseDirectoryChanged(bool chooseDirectory);

    void dialogPromptChanged(QString dialogPrompt);

    void optionChanged(QString option);

private:
    void updateLabel();

    QPushButton* m_changeButton;
    QPushButton* m_defaultButton;
    QString m_path;
    QString m_defaultPath;
    QLabel* m_label;
    QString m_labelPrefix;
    bool m_chooseDirectory = false;
    QString m_dialogPrompt;
    QString m_option;
};

class SettingsDateTime : public SettingsControl
{
    Q_OBJECT

    Q_PROPERTY(QDateTime value READ value WRITE setValue NOTIFY valueChanged)
public:
    SettingsDateTime(QWidget *pr = nullptr);

    virtual void saveState(QSettings& settings) const override;
    virtual void restoreState(QSettings& settings) override;

    virtual void setLabel(QString label) override;

    QDateTime value() const;

public slots:

    void setValue(QDateTime value);

signals:
    void valueChanged(QDateTime value);

private:
    QDateTimeEdit* m_edit;
    QLabel* m_label;
    QDateTime m_value;
};

#endif // SETTINGSWIDGETS_HXX
