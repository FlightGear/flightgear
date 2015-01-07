
#include <QDialog>
#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>

#include <Airports/airport.hxx>

namespace Ui
{
    class Launcher;
}

class AirportSearchModel;
class QModelIndex;
class AircraftProxyModel;
class QCheckBox;

class QtLauncher : public QDialog
{
    Q_OBJECT
public:
    QtLauncher();
    virtual ~QtLauncher();
    
    static bool runLauncherDialog();

private slots:
    void onRun();
    void onQuit();

    void onSearchAirports();

    void onAirportChanged();

    void onAirportChoiceSelected(const QModelIndex& index);
    void onAircraftSelected(const QModelIndex& index);

    void onPopupAirportHistory();
    void onPopupAircraftHistory();

    void onOpenCustomAircraftDir();

    void onEditRatingsFilter();

    void updateAirportDescription();
    void updateSettingsSummary();

    void onAirportSearchComplete();

    void onAddSceneryPath();
    void onRemoveSceneryPath();

    void onRembrandtToggled(bool b);
private:
    void setAirport(FGAirportRef ref);
    void updateSelectedAircraft();

    void restoreSettings();
    void saveSettings();
    
    QModelIndex proxyIndexForAircraftPath(QString path) const;
    QModelIndex sourceIndexForAircraftPath(QString path) const;

    void setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const;

    QScopedPointer<Ui::Launcher> m_ui;
    AirportSearchModel* m_airportsModel;
    AircraftProxyModel* m_aircraftProxy;

    FGAirportRef m_selectedAirport;

    QString m_selectedAircraft;
    QStringList m_recentAircraft,
        m_recentAirports;
    QString m_customAircraftDir;

    int m_ratingFilters[4];
};
