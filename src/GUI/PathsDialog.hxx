#ifndef PATHSDIALOG_HXX
#define PATHSDIALOG_HXX

#include <QDialog>

#include <simgear/package/Root.hxx>


namespace Ui {
class AddOnsPage;
}

class CatalogListModel;

class AddOnsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AddOnsPage(QWidget *parent, simgear::pkg::RootRef root);
    ~AddOnsPage();

signals:
    void downloadDirChanged();
    void sceneryPathsChanged();
    
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

    void onChangeDataDir();
private:
    void updateUi();
    void setDownloadDir();

    void saveAircraftPaths();
    void saveSceneryPaths();

    Ui::AddOnsPage* m_ui;
    CatalogListModel* m_catalogsModel;
    simgear::pkg::RootRef m_packageRoot;
    QString m_downloadDir;
    
};

#endif // PATHSDIALOG_HXX
