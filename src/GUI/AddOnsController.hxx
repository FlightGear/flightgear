#ifndef ADDONSCONTROLLER_HXX
#define ADDONSCONTROLLER_HXX

#include <QObject>

class CatalogListModel;
class LauncherMainWindow;

class AddOnsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList aircraftPaths READ aircraftPaths WRITE setAircraftPaths NOTIFY aircraftPathsChanged)
    Q_PROPERTY(QStringList sceneryPaths READ sceneryPaths WRITE setSceneryPaths NOTIFY sceneryPathsChanged)

    Q_PROPERTY(CatalogListModel* catalogs READ catalogs CONSTANT)

    Q_PROPERTY(bool isOfficialHangarRegistered READ isOfficialHangarRegistered NOTIFY isOfficialHangarRegisteredChanged)
    Q_PROPERTY(bool showNoOfficialHangar READ showNoOfficialHangar NOTIFY showNoOfficialHangarChanged)

public:
    explicit AddOnsController(LauncherMainWindow *parent = nullptr);

    QStringList aircraftPaths() const;
    QStringList sceneryPaths() const;

    CatalogListModel* catalogs() const
    { return m_catalogs; }

    Q_INVOKABLE QString addAircraftPath() const;
    Q_INVOKABLE QString addSceneryPath() const;

    // we would ideally do this in-page, but needs some extra work
    Q_INVOKABLE QString installCustomScenery();

    Q_INVOKABLE void openDirectory(QString path);

    bool isOfficialHangarRegistered();

    bool showNoOfficialHangar() const;

    Q_INVOKABLE void officialCatalogAction(QString s);
signals:
    void aircraftPathsChanged(QStringList aircraftPaths);
    void sceneryPathsChanged(QStringList sceneryPaths);

    void isOfficialHangarRegisteredChanged();
    void showNoOfficialHangarChanged();

public slots:
    void setAircraftPaths(QStringList aircraftPaths);
    void setSceneryPaths(QStringList sceneryPaths);

private:
    bool shouldShowOfficialCatalogMessage() const;
    void onCatalogsChanged();

    LauncherMainWindow* m_launcher;
    CatalogListModel* m_catalogs = nullptr;
    QStringList m_aircraftPaths;
    QStringList m_sceneryPaths;
};

#endif // ADDONSCONTROLLER_HXX
