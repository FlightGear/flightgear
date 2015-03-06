
#include <QDialog>
#include <QScopedPointer>
#include <QString>

namespace Ui
{
    class SetupRootDialog;
}

class SetupRootDialog : public QDialog
{
    Q_OBJECT
public:
    SetupRootDialog(bool usedDefaultPath);

    ~SetupRootDialog();

    static bool restoreUserSelectedRoot();
private slots:

    void onBrowse();

    void onDownload();

    void updatePromptText();
private:

    static bool validatePath(QString path);
    static bool validateVersion(QString path);

    enum PromptState
    {
        DefaultPathCheckFailed,
        ExplicitPathCheckFailed,
        VersionCheckFailed,
        ChoseInvalidLocation,
        ChoseInvalidVersion
    };

    PromptState m_promptState;
    QScopedPointer<Ui::SetupRootDialog> m_ui;
    QString m_browsedPath;
};