#ifndef PATHSDIALOG_HXX
#define PATHSDIALOG_HXX

#include <QDialog>

#include <simgear/package/Root.hxx>


namespace Ui {
class PathsDialog;
}

class CatalogListModel;

class PathsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PathsDialog(QWidget *parent, simgear::pkg::RootRef root);
    ~PathsDialog();

protected:
    virtual void accept();
    
private slots:
    void onAddSceneryPath();
    void onRemoveSceneryPath();

    void onAddAircraftPath();
    void onRemoveAircraftPath();

    void onAddCatalog();
    void onRemoveCatalog();
    void onAddDefaultCatalog();

    void onChangeDownloadDir();
    void onClearDownloadDir();
private:
    void updateUi();

    Ui::PathsDialog* m_ui;
    CatalogListModel* m_catalogsModel;
    simgear::pkg::RootRef m_packageRoot;
    QString m_downloadDir;
    
};

#endif // PATHSDIALOG_HXX
