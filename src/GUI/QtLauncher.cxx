#include "QtLauncher.hxx"

// Qt
#include <QProgressDialog>
#include <QCoreApplication>
#include <QAbstractListModel>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <QCompleter>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QListView>
#include <QSettings>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QMenu>

// Simgear
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include "ui_Launcher.h"

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx> // for parking
#include <Main/options.hxx>

using namespace flightgear;

const int MAX_RECENT_AIRPORTS = 20;

namespace { // anonymous namespace

void initNavCache()
{
    NavDataCache* cache = NavDataCache::instance();
    if (cache->isRebuildRequired()) {
        QProgressDialog rebuildProgress("Initialising navigation data, this may take several minutes",
                                       QString() /* cancel text */,
                                       0, 0);
        rebuildProgress.setWindowModality(Qt::WindowModal);
        rebuildProgress.show();

        while (!cache->rebuild()) {
            // sleep to give the rebuild thread more time
            SGTimeStamp::sleepForMSec(50);
            rebuildProgress.setValue(0);
            QCoreApplication::processEvents();
        }
    }
}

struct AircraftItem {
    QString path;
    QPixmap thumbnail;
    QString description;
    // rating, etc
};

class AircraftScanThread : public QThread
{
    Q_OBJECT
public:
    AircraftScanThread(QStringList dirsToScan) :
        m_dirs(dirsToScan)
    {

    }

    /** thread-safe access to items already scanned */
    QList<AircraftItem> items()
    {
        QList<AircraftItem> result;
        QMutexLocker g(&m_lock);
        result.swap(m_items);
        g.unlock();
        return result;
    }
Q_SIGNALS:
    void addedItems();

protected:
    virtual void run()
    {
        Q_FOREACH(QString d, m_dirs) {
            scanAircraftDir(QDir(d));
        }
    }

private:
    void scanAircraftDir(QDir path)
    {
        QStringList filters;
        filters << "*-set.xml";
        Q_FOREACH(QFileInfo child, path.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir childDir(child.absoluteFilePath());
            Q_FOREACH(QFileInfo xmlChild, childDir.entryInfoList(filters, QDir::Files)) {
                SGPropertyNode root;
                try {
                    readProperties(xmlChild.absoluteFilePath().toStdString(), &root);
                } catch (sg_exception& ) {
                    continue;
                }

                if (!root.hasChild("sim")) {
                    continue;
                }

                SGPropertyNode_ptr sim = root.getNode("sim");

                AircraftItem item;
                item.path = xmlChild.absoluteFilePath();
                item.description = sim->getStringValue("description");
                // authors
                // ratings
                if (childDir.exists("thumbnail.jpg")) {
                    item.thumbnail.load(childDir.filePath("thumbnail.jpg"));
                    // resize to the standard size
                    if (item.thumbnail.width() > 171) {
                        item.thumbnail = item.thumbnail.scaledToWidth(171);
                    }
                }

                {
                    QMutexLocker g(&m_lock);
                    m_items.append(item);
                }
            } // of set.xml iteration

            emit addedItems();
        } // of subdir iteration
    }

    QMutex m_lock;
    QStringList m_dirs;
    QList<AircraftItem> m_items;
};

class AircraftItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AircraftItemModel()
    {
        QStringList dirs;
        Q_FOREACH(std::string ap, globals->get_aircraft_paths()) {
            dirs << QString::fromStdString(ap);
        }

        SGPath rootAircraft(globals->get_fg_root());
        rootAircraft.append("Aircraft");
        dirs << QString::fromStdString(rootAircraft.str());

        m_scanThread = new AircraftScanThread(dirs);
        connect(m_scanThread, &AircraftScanThread::finished, this,
                &AircraftItemModel::onScanFinished);
        connect(m_scanThread, &AircraftScanThread::addedItems,
                this, &AircraftItemModel::onScanResults);
        m_scanThread->start();
    }

    virtual int rowCount(const QModelIndex& parent) const
    {
        return m_items.size();
    }

    virtual QVariant data(const QModelIndex& index, int role) const
    {
        const AircraftItem& item(m_items.at(index.row()));
        if (role == Qt::DisplayRole) {
            return item.description;
        } else if (role == Qt::DecorationRole) {
            return item.thumbnail;
        } else if (role == Qt::UserRole) {
            return item.path;
        }

        // rating
        // path

        return QVariant();
    }

private slots:
    void onScanResults()
    {
        QList<AircraftItem> newItems = m_scanThread->items();
        if (newItems.isEmpty())
            return;

        int firstRow = m_items.count();
        int lastRow = firstRow + newItems.count() - 1;
        beginInsertRows(QModelIndex(), firstRow, lastRow);
        m_items.append(newItems);
        endInsertRows();
    }

    void onScanFinished()
    {
        delete m_scanThread;
        m_scanThread = NULL;
    }

private:
    AircraftScanThread* m_scanThread;
    QList<AircraftItem> m_items;
};

class ICAOValidator : public QValidator
{
public:
    virtual State validate(QString& input, int& pos) const
    {
        if (input.size() < 2) {
            return QValidator::Intermediate;
        }

        // exact match check
        if (FGAirport::findByIdent(input.toStdString())) {
            return QValidator::Acceptable;
        }

        FGAirport::PortsFilter filter;
        FGPositionedList hits = NavDataCache::instance()->findAllWithIdent(input.toStdString(),
                                                                          &filter,
                                                                          false /* exact */);
        if (!hits.empty()) {
            return QValidator::Intermediate;
        }

        return QValidator::Invalid;
    }
private:
};

class ICAOAirportModel : public QAbstractListModel
{
public:
    ICAOAirportModel()
    {
        Q_FOREACH(std::string s, NavDataCache::instance()->getAllAirportCodes()) {
            m_airports.append(QString::fromStdString(s));
        }
    }

    virtual int rowCount(const QModelIndex&) const
    {
        return m_airports.size();
    }

    virtual QVariant data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
            return m_airports.at(index.row());
        }

        return QVariant();
    }

private:
    QStringList m_airports;
};

} // of anonymous namespace

class AirportSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AirportSearchModel()
    {
    }

    void setSearch(QString t)
    {
        beginResetModel();


        m_airports.clear();
        m_ids = NavDataCache::instance()->searchAirportNamesAndIdents2(t.toUpper().toStdString());
        PositionedIDVec::const_iterator it;
        for (it = m_ids.begin(); it != m_ids.end(); ++it) {
            m_airports.push_back(FGAirportRef()); // null ref
        }

        endResetModel();
    }

    virtual int rowCount(const QModelIndex&) const
    {
        // if empty, return 1 for special 'no matches'?
        return m_ids.size();
    }

    virtual QVariant data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();
        
        FGAirportRef apt = m_airports[index.row()];
        if (!apt.valid()) {
            apt = FGPositioned::loadById<FGAirport>(m_ids[index.row()]);
            m_airports[index.row()] = apt;
        }

        if (role == Qt::DisplayRole) {
            QString name = QString::fromStdString(apt->name());
            return QString("%1: %2").arg(QString::fromStdString(apt->ident())).arg(name);
        }

        if (role == Qt::EditRole) {
            return QString::fromStdString(apt->ident());
        }

        if (role == Qt::UserRole) {
            return m_ids[index.row()];
        }

        return QVariant();
    }

    QString firstIdent() const
    {
        if (m_ids.empty())
            return QString();

        if (!m_airports.front().valid()) {
            m_airports[0] = FGPositioned::loadById<FGAirport>(m_ids.front());
        }

        return QString::fromStdString(m_airports.front()->ident());
    }
private slots:
    void onSearch(QString t)
    {
        m_searchTerm = t;
        setSearch(t);
    }


private:
    QString m_searchTerm;
    PositionedIDVec m_ids;
    mutable std::vector<FGAirportRef> m_airports;
};

QtLauncher::QtLauncher() :
    QDialog(),
    m_ui(NULL)
{
    m_ui.reset(new Ui::Launcher);
    m_ui->setupUi(this);

    m_airportsList = new QListView;
    m_airportsList->setWindowFlags(Qt::Popup);
    m_airportsList->resize(m_ui->airportEdit->width(), 200);

    m_airportsModel = new AirportSearchModel;
    m_airportsList->setModel(m_airportsModel);

    connect(m_airportsList, &QListView::clicked,
            this, &QtLauncher::onAirportChoiceSelected);

    m_aircraftProxy = new QSortFilterProxyModel(this);
    m_aircraftProxy->setSourceModel(new AircraftItemModel);
    m_aircraftProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_ui->aircraftList->setModel(m_aircraftProxy);
    m_ui->aircraftList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_ui->aircraftList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->aircraftList, &QListView::clicked,
            this, &QtLauncher::onAircraftSelected);

    connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));
    connect(m_ui->airportEdit, SIGNAL(returnPressed()),
            this, SLOT(onSearchAirports()));

    connect(m_ui->aircraftFilter, &QLineEdit::textChanged,
            m_aircraftProxy, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_ui->airportHistory, &QPushButton::clicked,
            this, &QtLauncher::onPopupAirportHistory);
    restoreSettings();
}

QtLauncher::~QtLauncher()
{
    
}

bool QtLauncher::runLauncherDialog()
{
    // startup the nav-cache now. This pre-empts normal startup of
    // the cache, but no harm done. (Providing scenery paths are consistent)

    initNavCache();

    QtLauncher dlg;
    dlg.exec();
    if (dlg.result() != QDialog::Accepted) {
        return false;
    }

    return true;
}

void QtLauncher::restoreSettings()
{
    QSettings settings;
    m_ui->rembrandtCheckbox->setChecked(settings.value("enable-rembrandt", false).toBool());
    m_ui->terrasyncCheck->setChecked(settings.value("enable-terrasync", true).toBool());
    m_ui->fullScreenCheckbox->setChecked(settings.value("start-fullscreen", false).toBool());
    m_ui->msaaCheckbox->setChecked(settings.value("enable-msaa", false).toBool());
    m_ui->fetchRealWxrCheckbox->setChecked(settings.value("enable-realwx", true).toBool());

    // full paths to -set.xml files
    m_recentAircraft = settings.value("recent-aircraft").toStringList();

    if (!m_recentAircraft.empty()) {
        m_selectedAircraft = m_recentAircraft.front();

    } else {

    }

    // ICAO identifiers
    m_recentAirports = settings.value("recent-airports").toStringList();
}

void QtLauncher::onRun()
{
    accept();

    QSettings settings;
    settings.setValue("enable-rembrandt", m_ui->rembrandtCheckbox->isChecked());
    settings.setValue("enable-terrasync", m_ui->terrasyncCheck->isChecked());
    settings.setValue("enable-msaa", m_ui->msaaCheckbox->isChecked());
    settings.setValue("start-fullscreen", m_ui->fullScreenCheckbox->isChecked());
    settings.setValue("enable-realwx", m_ui->fetchRealWxrCheckbox->isChecked());

    flightgear::Options* opt = flightgear::Options::sharedInstance();
    if (m_ui->terrasyncCheck->isChecked()) {
        opt->addOption("enable-terrasync", "");
    } else {
        opt->addOption("disable-terrasync", "");
    }

    if (m_ui->fetchRealWxrCheckbox->isChecked()) {
        opt->addOption("enable-real-weather-fetch", "");
    } else {
        opt->addOption("disable-real-weather-fetch", "");
    }

    if (m_ui->rembrandtCheckbox->isChecked()) {
        opt->addOption("enable-rembrandt", "");
    } else {
        opt->addOption("disable-rembrandt", "");
    }

    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
        QFileInfo setFileInfo(m_selectedAircraft);
        opt->addOption("aircraft-dir", setFileInfo.dir().absolutePath().toStdString());
        QString setFile = setFileInfo.fileName();
        Q_ASSERT(setFile.endsWith("-set.xml"));
        setFile.truncate(setFile.count() - 8); // drop the '-set.xml' portion
        opt->addOption("aircraft", setFile.toStdString());

        // do history management
        m_recentAircraft.append(m_selectedAircraft);
    }

    // airport / location
    if (m_selectedAirport) {
        opt->addOption("airport", m_selectedAirport->ident());
    }

    if (m_ui->runwayRadio->isChecked()) {
        int index = m_ui->runwayCombo->currentData().toInt();
        if (index >= 0) {
            // explicit runway choice
            opt->addOption("runway", m_selectedAirport->getRunwayByIndex(index)->ident());
        }

        if (m_ui->onFinalCheckbox->isChecked()) {
            opt->addOption("glideslope", "3.0");
            opt->addOption("offset-distance", "10.0"); // in nautical miles
        }
    } else if (m_ui->parkingRadio->isChecked()) {
        // parking selection

    }
}

void QtLauncher::onQuit()
{
    reject();
}

void QtLauncher::onSearchAirports()
{
    QString search = m_ui->airportEdit->text();
    m_airportsModel->setSearch(search);

    int numResults = m_airportsModel->rowCount(QModelIndex());
    if (numResults > 1) {
        QPoint listPos = mapToGlobal(m_ui->airportEdit->geometry().bottomLeft());
        m_airportsList->move(listPos);
        m_airportsList->show();
        // install an app even filter so clicks outside cancel the search
    } else if (numResults == 1) {
        QString ident = m_airportsModel->firstIdent();
        setAirport(FGAirport::findByIdent(ident.toStdString()));
    } else {
        m_ui->airportEdit->clear();
        setAirport(FGAirportRef());
        m_ui->airportDescription->setText(QString("No matching airports for '%1'").arg(search));
    }
}

void QtLauncher::onAirportChanged()
{
    m_ui->runwayCombo->setEnabled(m_selectedAirport);
    m_ui->parkingCombo->setEnabled(m_selectedAirport);
    m_ui->airportDiagram->setAirport(m_selectedAirport);

    m_ui->runwayRadio->setChecked(true); // default back to runway mode
    // unelss multiplayer is enabled ?

    if (!m_selectedAirport) {
        m_ui->airportDescription->setText(QString());
        m_ui->airportDiagram->setEnabled(false);
        return;
    }

    m_ui->airportDiagram->setEnabled(true);
    QString ident = QString::fromStdString(m_selectedAirport->ident()),
        name = QString::fromStdString(m_selectedAirport->name());
    m_ui->airportDescription->setText(QString("%1 / %2").arg(ident).arg(name));

    m_ui->runwayCombo->clear();
    m_ui->runwayCombo->addItem("Automatic", -1);
    for (unsigned int r=0; r<m_selectedAirport->numRunways(); ++r) {
        FGRunwayRef rwy = m_selectedAirport->getRunwayByIndex(r);
        // add runway with index as data role
        m_ui->runwayCombo->addItem(QString::fromStdString(rwy->ident()), r);

        m_ui->airportDiagram->addRunway(rwy);
    }

    m_ui->parkingCombo->clear();
    FGAirportDynamics* dynamics = m_selectedAirport->getDynamics();
    PositionedIDVec parkings = NavDataCache::instance()->airportItemsOfType(
                                                                            m_selectedAirport->guid(),
                                                                            FGPositioned::PARKING);
    if (parkings.empty()) {
        m_ui->parkingCombo->setEnabled(false);
    } else {
        m_ui->parkingCombo->setEnabled(true);
        Q_FOREACH(PositionedID parking, parkings) {
            FGParking* park = dynamics->getParking(parking);
            m_ui->parkingCombo->addItem(QString::fromStdString(park->getName()), parking);

            m_ui->airportDiagram->addParking(park);
        }
    }
}

void QtLauncher::onAirportChoiceSelected(const QModelIndex& index)
{
    m_airportsList->hide();
    setAirport(FGPositioned::loadById<FGAirport>(index.data(Qt::UserRole).toULongLong()));
}

void QtLauncher::onAircraftSelected(const QModelIndex& index)
{
    m_selectedAircraft = index.data(Qt::UserRole).toString();
}

void QtLauncher::onPopupAirportHistory()
{
    QMenu m;
    Q_FOREACH(QString aptCode, m_recentAirports) {
        FGAirportRef apt = FGAirport::findByIdent(aptCode.toStdString());
        QString name = QString::fromStdString(apt->name());
        QAction* act = m.addAction(QString("%1 - %2").arg(aptCode).arg(name));
        act->setData(aptCode);
    }

    QPoint popupPos = m_ui->airportHistory->mapToGlobal(m_ui->airportHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        FGAirportRef apt = FGAirport::findByIdent(triggered->data().toString().toStdString());
        setAirport(apt);
    }
}

void QtLauncher::setAirport(FGAirportRef ref)
{
    if (m_selectedAirport == ref)
        return;

    m_selectedAirport = ref;
    onAirportChanged();

    if (ref.valid()) {
        // maintain the recent airport list
        QString icao = QString::fromStdString(ref->ident());
        if (m_recentAirports.contains(icao)) {
            // move to front
            m_recentAirports.removeOne(icao);
            m_recentAirports.push_front(icao);
        } else {
            // insert and trim list if necessary
            m_recentAirports.push_front(icao);
            if (m_recentAirports.size() > MAX_RECENT_AIRPORTS) {
                m_recentAirports.pop_back();
            }
        }
    }
}

#include "QtLauncher.moc"

