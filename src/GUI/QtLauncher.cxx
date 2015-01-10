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
#include <QDesktopServices>
#include <QUrl>
#include <QAction>
#include <QStyledItemDelegate>
#include <QLinearGradient>
#include <QFileDialog>
#include <QMessageBox>

// Simgear
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include "ui_Launcher.h"
#include "EditRatingsFilterDialog.hxx"

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx> // for parking
#include <Main/options.hxx>

using namespace flightgear;

const int MAX_RECENT_AIRPORTS = 32;
const int MAX_RECENT_AIRCRAFT = 20;

namespace { // anonymous namespace

const int AircraftPathRole = Qt::UserRole + 1;
const int AircraftAuthorsRole = Qt::UserRole + 2;
const int AircraftRatingRole = Qt::UserRole + 100;

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

struct AircraftItem
{
    AircraftItem() {
        // oh for C++11 initialisers
        for (int i=0; i<4; ++i) ratings[i] = 0;
    }

    AircraftItem(QDir dir, QString filePath)
    {
        for (int i=0; i<4; ++i) ratings[i] = 0;

        SGPropertyNode root;
        readProperties(filePath.toStdString(), &root);

        if (!root.hasChild("sim")) {
            throw sg_io_exception(std::string("Malformed -set.xml file"), filePath.toStdString());
        }

        SGPropertyNode_ptr sim = root.getNode("sim");

        path = filePath;
        description = sim->getStringValue("description");
        authors =  sim->getStringValue("author");

        if (sim->hasChild("rating")) {
            parseRatings(sim->getNode("rating"));
        }

        if (dir.exists("thumbnail.jpg")) {
            thumbnail.load(dir.filePath("thumbnail.jpg"));
            // resize to the standard size
            if (thumbnail.height() > 128) {
                thumbnail = thumbnail.scaledToHeight(128);
            }
        }

    }

    QString path;
    QPixmap thumbnail;
    QString description;
    QString authors;
    int ratings[4];

private:
    void parseRatings(SGPropertyNode_ptr ratingsNode)
    {
        ratings[0] = ratingsNode->getIntValue("FDM");
        ratings[1] = ratingsNode->getIntValue("systems");
        ratings[2] = ratingsNode->getIntValue("cockpit");
        ratings[3] = ratingsNode->getIntValue("model");
    }
};

class AircraftScanThread : public QThread
{
    Q_OBJECT
public:
    AircraftScanThread(QStringList dirsToScan) :
        m_dirs(dirsToScan),
        m_done(false)
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

    void setDone()
    {
        m_done = true;
    }
Q_SIGNALS:
    void addedItems();

protected:
    virtual void run()
    {
        Q_FOREACH(QString d, m_dirs) {
            scanAircraftDir(QDir(d));
            if (m_done) {
                return;
            }
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
                try {
                    AircraftItem item(childDir, xmlChild.absoluteFilePath());
                    // lock mutex whil we modify the items array
                    {
                        QMutexLocker g(&m_lock);
                        m_items.append(item);
                    }
                } catch (sg_exception& e) {
                    continue;
                }

                if (m_done) {
                    return;
                }
            } // of set.xml iteration

            emit addedItems();
        } // of subdir iteration
    }

    QMutex m_lock;
    QStringList m_dirs;
    QList<AircraftItem> m_items;
    bool m_done;
};

class AircraftItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AircraftItemModel(QObject* pr) :
        QAbstractListModel(pr)
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

    ~AircraftItemModel()
    {
        if (m_scanThread) {
            m_scanThread->setDone();
            m_scanThread->wait(1000);
            delete m_scanThread;
        }
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
        } else if (role == AircraftPathRole) {
            return item.path;
        } else if (role == AircraftAuthorsRole) {
            return item.authors;
        } else if (role >= AircraftRatingRole) {
            return item.ratings[role - AircraftRatingRole];
        } else if (role == Qt::ToolTipRole) {
            return item.path;
        }

        return QVariant();
    }

  QModelIndex indexOfAircraftPath(QString path) const
  {
      for (int row=0; row <m_items.size(); ++row) {
          const AircraftItem& item(m_items.at(row));
          if (item.path == path) {
              return index(row);
          }
      }

      return QModelIndex();
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

class AircraftItemDelegate : public QStyledItemDelegate
{
public:
    static const int MARGIN = 4;

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        // selection feedback rendering
        if (option.state & QStyle::State_Selected) {
            QLinearGradient grad(option.rect.topLeft(), option.rect.bottomLeft());
            grad.setColorAt(0.0, QColor(152, 163, 180));
            grad.setColorAt(1.0, QColor(90, 107, 131));

            QBrush backgroundBrush(grad);
            painter->fillRect(option.rect, backgroundBrush);

            painter->setPen(QColor(90, 107, 131));
            painter->drawLine(option.rect.topLeft(), option.rect.topRight());

        }

        QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

        QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
        painter->drawPixmap(contentRect.topLeft(), thumbnail);

        // draw 1px frame
        painter->setPen(QColor(0x7f, 0x7f, 0x7f));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(contentRect.left(), contentRect.top(), thumbnail.width(), thumbnail.height());

        QString description = index.data(Qt::DisplayRole).toString();
        contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

        painter->setPen(Qt::black);
        QFont f;
        f.setPointSize(18);
        painter->setFont(f);

        QRect actualBounds;
        painter->drawText(contentRect, Qt::TextWordWrap, description, &actualBounds);

        QString authors = index.data(AircraftAuthorsRole).toString();

        f.setPointSize(12);
        painter->setFont(f);

        QRect authorsRect = contentRect;
        authorsRect.moveTop(actualBounds.bottom() + MARGIN);
        painter->drawText(authorsRect, Qt::TextWordWrap,
                          QString("by: %1").arg(authors),
                          &actualBounds);

        QRect r = contentRect;
        r.setWidth(contentRect.width() / 2);
        r.moveTop(actualBounds.bottom() + MARGIN);
        r.setHeight(24);

        drawRating(painter, "Flight model:", r, index.data(AircraftRatingRole).toInt());
        r.moveTop(r.bottom());
        drawRating(painter, "Systems:", r, index.data(AircraftRatingRole + 1).toInt());

        r.moveTop(actualBounds.bottom() + MARGIN);
        r.moveLeft(r.right());
        drawRating(painter, "Cockpit:", r, index.data(AircraftRatingRole + 2).toInt());
        r.moveTop(r.bottom());
        drawRating(painter, "Exterior model:", r, index.data(AircraftRatingRole + 3).toInt());


    }

    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
    {
        return QSize(500, 128 + (MARGIN * 2));
    }

private:
    void drawRating(QPainter* painter, QString label, const QRect& box, int value) const
    {
        const int DOT_SIZE = 10;
        const int DOT_MARGIN = 4;

        QRect dotBox = box;
        dotBox.setLeft(box.right() - (DOT_MARGIN * 6 + DOT_SIZE * 5));

        painter->setPen(Qt::black);
        QRect textBox = box;
        textBox.setRight(dotBox.left() - DOT_MARGIN);
        painter->drawText(textBox, Qt::AlignVCenter | Qt::AlignRight, label);

        painter->setPen(Qt::NoPen);
        QRect dot(dotBox.left() + DOT_MARGIN,
                  dotBox.center().y() - (DOT_SIZE / 2),
                  DOT_SIZE,
                  DOT_SIZE);
        for (int i=0; i<5; ++i) {
            painter->setBrush((i < value) ? QColor(0x3f, 0x3f, 0x3f) : QColor(0xaf, 0xaf, 0xaf));
            painter->drawEllipse(dot);
            dot.moveLeft(dot.right() + DOT_MARGIN);
        }
    }
};

} // of anonymous namespace

class AirportSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AirportSearchModel() :
        m_searchActive(false)
    {
    }

    void setSearch(QString t)
    {
        beginResetModel();

        m_airports.clear();
        m_ids.clear();

        std::string term(t.toUpper().toStdString());
        // try ICAO lookup first
        FGAirportRef ref = FGAirport::findByIdent(term);
        if (ref) {
            m_ids.push_back(ref->guid());
            m_airports.push_back(ref);
        } else {
            m_search.reset(new NavDataCache::ThreadedAirportSearch(term));
            QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
            m_searchActive = true;
        }

        endResetModel();
    }

    bool isSearchActive() const
    {
        return m_searchActive;
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
            return static_cast<qlonglong>(m_ids[index.row()]);
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

Q_SIGNALS:
    void searchComplete();

private slots:
    void onSearchResultsPoll()
    {
        PositionedIDVec newIds = m_search->results();
        
        beginInsertRows(QModelIndex(), m_ids.size(), newIds.size() - 1);
        for (unsigned int i=m_ids.size(); i < newIds.size(); ++i) {
            m_ids.push_back(newIds[i]);
            m_airports.push_back(FGAirportRef()); // null ref
        }
        endInsertRows();

        if (m_search->isComplete()) {
            m_searchActive = false;
            m_search.reset();
            emit searchComplete();
        } else {
            QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
        }
    }

private:
    PositionedIDVec m_ids;
    mutable std::vector<FGAirportRef> m_airports;
    bool m_searchActive;
    QScopedPointer<NavDataCache::ThreadedAirportSearch> m_search;
};

class AircraftProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AircraftProxyModel(QObject* pr) :
        QSortFilterProxyModel(pr),
        m_ratingsFilter(true)
    {
        for (int i=0; i<4; ++i) {
            m_ratings[i] = 3;
        }
    }

    void setRatings(int* ratings)
    {
        ::memcpy(m_ratings, ratings, sizeof(int) * 4);
        invalidate();
    }

public slots:
    void setRatingFilterEnabled(bool e)
    {
        if (e == m_ratingsFilter) {
            return;
        }

        m_ratingsFilter = e;
        invalidate();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
            return false;
        }

        if (m_ratingsFilter) {
            QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
            for (int i=0; i<4; ++i) {
                if (m_ratings[i] > index.data(AircraftRatingRole + i).toInt()) {
                    return false;
                }
            }
        }

        return true;
    }

private:
    bool m_ratingsFilter;
    int m_ratings[4];
};

QtLauncher::QtLauncher() :
    QDialog(),
    m_ui(NULL)
{
    m_ui.reset(new Ui::Launcher);
    m_ui->setupUi(this);

    for (int i=0; i<4; ++i) {
        m_ratingFilters[i] = 3;
    }

    m_airportsModel = new AirportSearchModel;
    m_ui->searchList->setModel(m_airportsModel);
    connect(m_ui->searchList, &QListView::clicked,
            this, &QtLauncher::onAirportChoiceSelected);
    connect(m_airportsModel, &AirportSearchModel::searchComplete,
            this, &QtLauncher::onAirportSearchComplete);

    SGPath p = SGPath::documents();
    p.append("FlightGear");
    p.append("Aircraft");
    m_customAircraftDir = QString::fromStdString(p.str());
    m_ui->customAircraftDirLabel->setText(QString("Custom aircraft folder: %1").arg(m_customAircraftDir));

    globals->append_aircraft_path(m_customAircraftDir.toStdString());

    // create and configure the proxy model
    m_aircraftProxy = new AircraftProxyModel(this);
    m_aircraftProxy->setSourceModel(new AircraftItemModel(this));

    m_aircraftProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_aircraftProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_aircraftProxy->setSortRole(Qt::DisplayRole);
    m_aircraftProxy->setDynamicSortFilter(true);

    m_ui->aircraftList->setModel(m_aircraftProxy);
    m_ui->aircraftList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ui->aircraftList->setItemDelegate(new AircraftItemDelegate);
    m_ui->aircraftList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->aircraftList, &QListView::clicked,
            this, &QtLauncher::onAircraftSelected);

    connect(m_ui->runwayCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->parkingCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->runwayRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->parkingRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->onFinalCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));


    connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));
    connect(m_ui->airportEdit, SIGNAL(returnPressed()),
            this, SLOT(onSearchAirports()));

    connect(m_ui->aircraftFilter, &QLineEdit::textChanged,
            m_aircraftProxy, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_ui->airportHistory, &QPushButton::clicked,
            this, &QtLauncher::onPopupAirportHistory);
    connect(m_ui->aircraftHistory, &QPushButton::clicked,
          this, &QtLauncher::onPopupAircraftHistory);

    restoreSettings();

    connect(m_ui->openAircraftDirButton, &QPushButton::clicked,
          this, &QtLauncher::onOpenCustomAircraftDir);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &QtLauncher::onQuit);
    addAction(qa);

    connect(m_ui->editRatingFilter, &QPushButton::clicked,
            this, &QtLauncher::onEditRatingsFilter);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setRatingFilterEnabled);

    QIcon historyIcon(":/history-icon");
    m_ui->aircraftHistory->setIcon(historyIcon);
    m_ui->airportHistory->setIcon(historyIcon);

    m_ui->searchIcon->setPixmap(QPixmap(":/search-icon"));

    connect(m_ui->timeOfDayCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->seasonCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->fetchRealWxrCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->rembrandtCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->terrasyncCheck, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->startPausedCheck, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->msaaCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));

    connect(m_ui->rembrandtCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(onRembrandtToggled(bool)));

    updateSettingsSummary();

    connect(m_ui->addSceneryPath, &QToolButton::clicked,
            this, &QtLauncher::onAddSceneryPath);
    connect(m_ui->removeSceneryPath, &QToolButton::clicked,
            this, &QtLauncher::onRemoveSceneryPath);
}

QtLauncher::~QtLauncher()
{
    
}

bool QtLauncher::runLauncherDialog()
{
     Q_INIT_RESOURCE(resources);

    // startup the nav-cache now. This pre-empts normal startup of
    // the cache, but no harm done. (Providing scenery paths are consistent)

    initNavCache();

  // setup scenery paths now, especially TerraSync path for airport
  // parking locations (after they're downloaded)

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
    m_ui->startPausedCheck->setChecked(settings.value("start-paused", false).toBool());
    m_ui->timeOfDayCombo->setCurrentIndex(settings.value("timeofday", 0).toInt());
    m_ui->seasonCombo->setCurrentIndex(settings.value("season", 0).toInt());

    // full paths to -set.xml files
    m_recentAircraft = settings.value("recent-aircraft").toStringList();

    if (!m_recentAircraft.empty()) {
        m_selectedAircraft = m_recentAircraft.front();
    } else {
        // select the default C172p
    }

    updateSelectedAircraft();

    // ICAO identifiers
    m_recentAirports = settings.value("recent-airports").toStringList();
    if (!m_recentAirports.empty()) {
        setAirport(FGAirport::findByIdent(m_recentAirports.front().toStdString()));
    }
    updateAirportDescription();

    // rating filters
    m_ui->ratingsFilterCheck->setChecked(settings.value("ratings-filter", true).toBool());
    int index = 0;
    Q_FOREACH(QVariant v, settings.value("min-ratings").toList()) {
        m_ratingFilters[index++] = v.toInt();
    }

    m_aircraftProxy->setRatingFilterEnabled(m_ui->ratingsFilterCheck->isChecked());
    m_aircraftProxy->setRatings(m_ratingFilters);

    QStringList sceneryPaths = settings.value("scenery-paths").toStringList();
    m_ui->sceneryPathsList->addItems(sceneryPaths);
}

void QtLauncher::saveSettings()
{
    QSettings settings;
    settings.setValue("enable-rembrandt", m_ui->rembrandtCheckbox->isChecked());
    settings.setValue("enable-terrasync", m_ui->terrasyncCheck->isChecked());
    settings.setValue("enable-msaa", m_ui->msaaCheckbox->isChecked());
    settings.setValue("start-fullscreen", m_ui->fullScreenCheckbox->isChecked());
    settings.setValue("enable-realwx", m_ui->fetchRealWxrCheckbox->isChecked());
    settings.setValue("start-paused", m_ui->startPausedCheck->isChecked());
    settings.setValue("ratings-filter", m_ui->ratingsFilterCheck->isChecked());
    settings.setValue("recent-aircraft", m_recentAircraft);
    settings.setValue("recent-airports", m_recentAirports);
    settings.setValue("timeofday", m_ui->timeOfDayCombo->currentIndex());
    settings.setValue("season", m_ui->seasonCombo->currentIndex());

    QStringList paths;
    for (int i=0; i<m_ui->sceneryPathsList->count(); ++i) {
        paths.append(m_ui->sceneryPathsList->item(i)->text());
    }

    settings.setValue("scenery-paths", paths);
}

void QtLauncher::setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
    std::string stdName(name.toStdString());
    if (cbox->isChecked()) {
        opt->addOption("enable-" + stdName, "");
    } else {
        opt->addOption("disable-" + stdName, "");
    }
}

void QtLauncher::onRun()
{
    accept();

    flightgear::Options* opt = flightgear::Options::sharedInstance();
    setEnableDisableOptionFromCheckbox(m_ui->terrasyncCheck, "terrasync");
    setEnableDisableOptionFromCheckbox(m_ui->fetchRealWxrCheckbox, "real-weather-fetch");
    setEnableDisableOptionFromCheckbox(m_ui->rembrandtCheckbox, "rembrandt");
    setEnableDisableOptionFromCheckbox(m_ui->fullScreenCheckbox, "fullscreen");
    setEnableDisableOptionFromCheckbox(m_ui->startPausedCheck, "freeze");

    // MSAA is more complex
    if (!m_ui->rembrandtCheckbox->isChecked()) {
        if (m_ui->msaaCheckbox->isChecked()) {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 1);
            globals->get_props()->setIntValue("/sim/rendering/multi-samples", 4);
        } else {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 0);
        }
    }

    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
        QFileInfo setFileInfo(m_selectedAircraft);
        opt->addOption("aircraft-dir", setFileInfo.dir().absolutePath().toStdString());
        QString setFile = setFileInfo.fileName();
        Q_ASSERT(setFile.endsWith("-set.xml"));
        setFile.truncate(setFile.count() - 8); // drop the '-set.xml' portion
        opt->addOption("aircraft", setFile.toStdString());

      // manage aircraft history
        if (m_recentAircraft.contains(m_selectedAircraft))
          m_recentAircraft.removeOne(m_selectedAircraft);
        m_recentAircraft.prepend(m_selectedAircraft);
        if (m_recentAircraft.size() > MAX_RECENT_AIRCRAFT)
          m_recentAircraft.pop_back();
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
        opt->addOption("parkpos", m_ui->parkingCombo->currentText().toStdString());
    }

    // time of day
    if (m_ui->timeOfDayCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("timeofday", dayval.toStdString());
    }

    if (m_ui->seasonCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("season", dayval.toStdString());
    }

    // scenery paths
    for (int i=0; i<m_ui->sceneryPathsList->count(); ++i) {
        QString path = m_ui->sceneryPathsList->item(i)->text();
        opt->addOption("fg-scenery", path.toStdString());
    }

    saveSettings();
}

void QtLauncher::onQuit()
{
    reject();
}

void QtLauncher::onSearchAirports()
{
    QString search = m_ui->airportEdit->text();
    m_airportsModel->setSearch(search);

    if (m_airportsModel->isSearchActive()) {
        m_ui->searchStatusText->setText(QString("Searching for '%1'").arg(search));
        m_ui->locationStack->setCurrentIndex(2);
    } else if (m_airportsModel->rowCount(QModelIndex()) == 1) {
        QString ident = m_airportsModel->firstIdent();
        setAirport(FGAirport::findByIdent(ident.toStdString()));
        m_ui->locationStack->setCurrentIndex(0);
    }
}

void QtLauncher::onAirportSearchComplete()
{
    int numResults = m_airportsModel->rowCount(QModelIndex());
    if (numResults == 0) {
        m_ui->searchStatusText->setText(QString("No matching airports for '%1'").arg(m_ui->airportEdit->text()));
    } else if (numResults == 1) {
        QString ident = m_airportsModel->firstIdent();
        setAirport(FGAirport::findByIdent(ident.toStdString()));
        m_ui->locationStack->setCurrentIndex(0);
    } else {
        m_ui->locationStack->setCurrentIndex(1);
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
        m_ui->parkingRadio->setEnabled(false);
    } else {
        m_ui->parkingCombo->setEnabled(true);
        m_ui->parkingRadio->setEnabled(true);
        Q_FOREACH(PositionedID parking, parkings) {
            FGParking* park = dynamics->getParking(parking);
            m_ui->parkingCombo->addItem(QString::fromStdString(park->getName()),
                                        static_cast<qlonglong>(parking));

            m_ui->airportDiagram->addParking(park);
        }
    }
}

void QtLauncher::updateAirportDescription()
{
    if (!m_selectedAirport) {
        m_ui->airportDescription->setText(QString("No airport selected"));
        return;
    }

    QString ident = QString::fromStdString(m_selectedAirport->ident()),
        name = QString::fromStdString(m_selectedAirport->name());
    QString locationOnAirport;
    if (m_ui->runwayRadio->isChecked()) {
        bool onFinal = m_ui->onFinalCheckbox->isChecked();
        QString runwayName = (m_ui->runwayCombo->currentIndex() == 0) ?
            "active runway" :
            QString("runway %1").arg(m_ui->runwayCombo->currentText());

        if (onFinal) {
            locationOnAirport = QString("on 10-mile final to %1").arg(runwayName);
        } else {
            locationOnAirport = QString("on %1").arg(runwayName);
        }
    } else if (m_ui->parkingRadio->isChecked()) {
        locationOnAirport =  QString("at parking position %1").arg(m_ui->parkingCombo->currentText());
    }

    m_ui->airportDescription->setText(QString("%2 (%1): %3").arg(ident).arg(name).arg(locationOnAirport));
}

void QtLauncher::onAirportChoiceSelected(const QModelIndex& index)
{
    m_ui->locationStack->setCurrentIndex(0);
    setAirport(FGPositioned::loadById<FGAirport>(index.data(Qt::UserRole).toULongLong()));
}

void QtLauncher::onAircraftSelected(const QModelIndex& index)
{
    m_selectedAircraft = index.data(AircraftPathRole).toString();
    updateSelectedAircraft();
}

void QtLauncher::updateSelectedAircraft()
{
    try {
        QFileInfo info(m_selectedAircraft);
        AircraftItem item(info.dir(), m_selectedAircraft);
        m_ui->thumbnail->setPixmap(item.thumbnail);
        m_ui->aircraftDescription->setText(item.description);
    } catch (sg_exception& e) {
        m_ui->thumbnail->setPixmap(QPixmap());
        m_ui->aircraftDescription->setText("");
    }
}

void QtLauncher::onPopupAirportHistory()
{
    if (m_recentAirports.isEmpty()) {
        return;
    }

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
        m_ui->airportEdit->clear();
        m_ui->locationStack->setCurrentIndex(0);
    }
}

QModelIndex QtLauncher::proxyIndexForAircraftPath(QString path) const
{
  return m_aircraftProxy->mapFromSource(sourceIndexForAircraftPath(path));
}

QModelIndex QtLauncher::sourceIndexForAircraftPath(QString path) const
{
    AircraftItemModel* sourceModel = qobject_cast<AircraftItemModel*>(m_aircraftProxy->sourceModel());
    Q_ASSERT(sourceModel);
    return sourceModel->indexOfAircraftPath(path);
}

void QtLauncher::onPopupAircraftHistory()
{
    if (m_recentAircraft.isEmpty()) {
        return;
    }

    QMenu m;
    Q_FOREACH(QString path, m_recentAircraft) {
        QModelIndex index = sourceIndexForAircraftPath(path);
        if (!index.isValid()) {
            // not scanned yet
            continue;
        }
        QAction* act = m.addAction(index.data(Qt::DisplayRole).toString());
        act->setData(path);
    }

    QPoint popupPos = m_ui->aircraftHistory->mapToGlobal(m_ui->aircraftHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        m_selectedAircraft = triggered->data().toString();
        QModelIndex index = proxyIndexForAircraftPath(m_selectedAircraft);
        m_ui->aircraftList->selectionModel()->setCurrentIndex(index,
                                                              QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftFilter->clear();
        updateSelectedAircraft();
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

    updateAirportDescription();
}

void QtLauncher::onOpenCustomAircraftDir()
{
    QFileInfo info(m_customAircraftDir);
    if (!info.exists()) {
        int result = QMessageBox::question(this, "Create folder?",
                                           "The custom aircraft folder does not exist, create it now?",
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::Yes);
        if (result == QMessageBox::No) {
            return;
        }

        QDir d(m_customAircraftDir);
        d.mkpath(m_customAircraftDir);
    }

  QUrl u = QUrl::fromLocalFile(m_customAircraftDir);
  QDesktopServices::openUrl(u);
}

void QtLauncher::onEditRatingsFilter()
{
    EditRatingsFilterDialog dialog(this);
    dialog.setRatings(m_ratingFilters);

    dialog.exec();
    if (dialog.result() == QDialog::Accepted) {
        QVariantList vl;
        for (int i=0; i<4; ++i) {
            m_ratingFilters[i] = dialog.getRating(i);
            vl.append(m_ratingFilters[i]);
        }
        m_aircraftProxy->setRatings(m_ratingFilters);

        QSettings settings;
        settings.setValue("min-ratings", vl);
    }
}

void QtLauncher::updateSettingsSummary()
{
    QStringList summary;
    if (m_ui->timeOfDayCombo->currentIndex() > 0) {
        summary.append(QString(m_ui->timeOfDayCombo->currentText().toLower()));
    }

    if (m_ui->seasonCombo->currentIndex() > 0) {
        summary.append(QString(m_ui->seasonCombo->currentText().toLower()));
    }

    if (m_ui->rembrandtCheckbox->isChecked()) {
        summary.append("Rembrandt enabled");
    } else if (m_ui->msaaCheckbox->isChecked()) {
        summary.append("anti-aliasing");
    }

    if (m_ui->fetchRealWxrCheckbox->isChecked()) {
        summary.append("live weather");
    }

    if (m_ui->terrasyncCheck->isChecked()) {
        summary.append("automatic scenery downloads");
    }

    if (m_ui->startPausedCheck->isChecked()) {
        summary.append("paused");
    }

    QString s = summary.join(", ");
    s[0] = s[0].toUpper();
    m_ui->settingsDescription->setText(s);
}

void QtLauncher::onAddSceneryPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Choose scenery folder"));
    if (!path.isEmpty()) {
        m_ui->sceneryPathsList->addItem(path);
        saveSettings();
    }
}

void QtLauncher::onRemoveSceneryPath()
{
    if (m_ui->sceneryPathsList->currentItem()) {
        delete m_ui->sceneryPathsList->currentItem();
        saveSettings();
    }
}

void QtLauncher::onRembrandtToggled(bool b)
{
    // Rembrandt and multi-sample are exclusive
    m_ui->msaaCheckbox->setEnabled(!b);
}

#include "QtLauncher.moc"

