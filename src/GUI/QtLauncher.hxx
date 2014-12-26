
#include <QDialog>
#include <QScopedPointer>
#include <QStringList>

#include <Airports/airport.hxx>

namespace Ui
{
    class Launcher;
}

class AirportSearchModel;
class QListView;
class QModelIndex;
class QSortFilterProxyModel;

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
private:
    void setAirport(FGAirportRef ref);

    void restoreSettings();
    
    QScopedPointer<Ui::Launcher> m_ui;
    AirportSearchModel* m_airportsModel;

    QSortFilterProxyModel* m_aircraftProxy;

    FGAirportRef m_selectedAirport;
    QListView* m_airportsList;

    QString m_selectedAircraft;
    QStringList m_recentAircraft,
        m_recentAirports;
};
