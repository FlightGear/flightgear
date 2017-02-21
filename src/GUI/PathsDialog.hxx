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

    static void addDefaultCatalog(QWidget* pr, bool silent);
    
signals:
    void sceneryPathsChanged();
    void aircraftPathsChanged();

private slots:
    void onAddSceneryPath();
    void onRemoveSceneryPath();

    void onAddAircraftPath();
    void onRemoveAircraftPath();

    void onAddCatalog();
    void onRemoveCatalog();
    void onAddDefaultCatalog();

    void onInstallScenery();
private:
    void updateUi();

    void saveAircraftPaths();
    void saveSceneryPaths();
    bool haveSceneryPath(QString path) const;

    Ui::AddOnsPage* m_ui;
    CatalogListModel* m_catalogsModel;
    simgear::pkg::RootRef m_packageRoot;
    
};

#endif // PATHSDIALOG_HXX
