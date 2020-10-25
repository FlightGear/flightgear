#ifndef LAUNCHERPACKAGEDELEGATE_HXX
#define LAUNCHERPACKAGEDELEGATE_HXX

#include <string>

#include <QObject>

#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>

class LauncherPackageDelegate : public QObject,
                                public simgear::pkg::Delegate
{
    Q_OBJECT

public:
    explicit LauncherPackageDelegate(QObject* parent = nullptr);
    ~LauncherPackageDelegate();

protected:
    void catalogRefreshed(simgear::pkg::CatalogRef aCatalog, StatusCode aReason) override;

    // mandatory overrides, not actually needed here.
    void startInstall(simgear::pkg::InstallRef) override {}
    void installProgress(simgear::pkg::InstallRef, unsigned int, unsigned int) override{};
    void finishInstall(simgear::pkg::InstallRef ref, StatusCode status) override;

signals:
    void didMigrateOfficialHangarChanged();

private:
    std::string _defaultCatalogId;
};

#endif // LAUNCHERPACKAGEDELEGATE_HXX
