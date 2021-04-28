#ifndef ADDONSCONTROLLER_HXX
#define ADDONSCONTROLLER_HXX

#include <QObject>
#include <QStringList>

class CatalogListModel;
class AddonsModel;
class LauncherMainWindow;
class PathListModel;
class LaunchConfig;

class AddOnsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(PathListModel* aircraftPaths READ aircraftPaths CONSTANT)
    Q_PROPERTY(PathListModel* sceneryPaths READ sceneryPaths CONSTANT)
    Q_PROPERTY(QStringList modulePaths READ modulePaths WRITE setModulePaths NOTIFY modulePathsChanged)

    Q_PROPERTY(CatalogListModel* catalogs READ catalogs CONSTANT)
    Q_PROPERTY(AddonsModel* modules READ modules NOTIFY modulesChanged)
    Q_PROPERTY(bool isOfficialHangarRegistered READ isOfficialHangarRegistered NOTIFY isOfficialHangarRegisteredChanged)
    Q_PROPERTY(bool showNoOfficialHangar READ showNoOfficialHangar NOTIFY showNoOfficialHangarChanged)
    Q_PROPERTY(bool havePathsFromCommandLine READ havePathsFromCommandLine CONSTANT)

public:
    explicit AddOnsController(LauncherMainWindow *parent, LaunchConfig* config);

    PathListModel* aircraftPaths() const;
    PathListModel* sceneryPaths() const;
    QStringList modulePaths() const;

    CatalogListModel* catalogs() const
    { return m_catalogs; }

    AddonsModel* modules() const
    { return m_addonsModuleModel; }

    Q_INVOKABLE QString addAircraftPath() const;
    Q_INVOKABLE QString addSceneryPath() const;
    Q_INVOKABLE QString addAddOnModulePath() const;

    // we would ideally do this in-page, but needs some extra work
    Q_INVOKABLE QString installCustomScenery();

    Q_INVOKABLE void openDirectory(QString path);

    bool isOfficialHangarRegistered();

    bool showNoOfficialHangar() const;

    Q_INVOKABLE void officialCatalogAction(QString s);

    bool havePathsFromCommandLine() const;
signals:
    void modulePathsChanged(QStringList modulePaths);
    void modulesChanged();

    void isOfficialHangarRegisteredChanged();
    void showNoOfficialHangarChanged();

public slots:
    void setModulePaths(QStringList modulePaths);
    void setAddons(AddonsModel* addons);
    void onAddonsChanged(void);

    void collectArgs();

private:
    void setLocalAircraftPaths();
    bool shouldShowOfficialCatalogMessage() const;
    void onCatalogsChanged();

    LauncherMainWindow* m_launcher;
    CatalogListModel* m_catalogs = nullptr;
    AddonsModel* m_addonsModuleModel = nullptr;
    LaunchConfig* m_config = nullptr;

    PathListModel* m_aircraftPaths = nullptr;
    PathListModel* m_sceneryPaths = nullptr;
    QStringList m_addonModulePaths;
};

#endif // ADDONSCONTROLLER_HXX
