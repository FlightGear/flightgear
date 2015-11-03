// LocationWidget.cxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started October 2015.
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "LocationWidget.hxx"
#include "ui_LocationWidget.h"

#include <QSettings>
#include <QAbstractListModel>
#include <QTimer>
#include <QDebug>
#include <QToolButton>

#include "AirportDiagram.hxx"
#include "NavaidDiagram.hxx"

#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx> // for parking
#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>

const int MAX_RECENT_AIRPORTS = 32;

using namespace flightgear;

QString fixNavaidName(QString s)
{
    // split into words
    QStringList words = s.split(QChar(' '));
    QStringList changedWords;
    Q_FOREACH(QString w, words) {
        QString up = w.toUpper();

        // expand common abbreviations
        if (up == "FLD") {
            changedWords.append("Field");
            continue;
        }

        if (up == "MUNI") {
            changedWords.append("Municipal");
            continue;
        }

        if (up == "RGNL") {
            changedWords.append("Regional");
            continue;
        }

        if (up == "CTR") {
            changedWords.append("Center");
            continue;
        }

        if (up == "INTL") {
            changedWords.append("International");
            continue;
        }

        // occurs in many Australian airport names in our DB
        if (up == "(NSW)") {
            changedWords.append("(New South Wales)");
            continue;
        }

        if ((up == "VOR") || (up == "NDB") || (up == "VOR-DME") || (up == "VORTAC") || (up == "NDB-DME")) {
            changedWords.append(w);
            continue;
        }

        QChar firstChar = w.at(0).toUpper();
        w = w.mid(1).toLower();
        w.prepend(firstChar);

        changedWords.append(w);
    }

    return changedWords.join(QChar(' '));
}

QString formatGeodAsString(const SGGeod& geod)
{
    QChar ns = (geod.getLatitudeDeg() > 0.0) ? 'N' : 'S';
    QChar ew = (geod.getLongitudeDeg() > 0.0) ? 'E' : 'W';

    return QString::number(fabs(geod.getLongitudeDeg()), 'f',2 ) + ew + " " +
            QString::number(fabs(geod.getLatitudeDeg()), 'f',2 ) + ns;
}

class IdentSearchFilter : public FGPositioned::TypeFilter
{
public:
    IdentSearchFilter()
    {
        addType(FGPositioned::AIRPORT);
        addType(FGPositioned::SEAPORT);
        addType(FGPositioned::HELIPAD);
        addType(FGPositioned::VOR);
        addType(FGPositioned::FIX);
        addType(FGPositioned::NDB);
    }
};

class NavSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    NavSearchModel() :
        m_searchActive(false)
    {
    }

    void setSearch(QString t)
    {
        beginResetModel();

        m_items.clear();
        m_ids.clear();

        std::string term(t.toUpper().toStdString());

        IdentSearchFilter filter;
        FGPositionedList exactMatches = NavDataCache::instance()->findAllWithIdent(term, &filter, true);

        for (unsigned int i=0; i<exactMatches.size(); ++i) {
            m_ids.push_back(exactMatches[i]->guid());
            m_items.push_back(exactMatches[i]);
        }
        endResetModel();


        m_search.reset(new NavDataCache::ThreadedGUISearch(term));
        QTimer::singleShot(100, this, &NavSearchModel::onSearchResultsPoll);
        m_searchActive = true;
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

        FGPositionedRef pos = itemAtRow(index.row());
        if (role == Qt::DisplayRole) {
            if (pos->type() == FGPositioned::FIX) {
                // fixes don't have a name, show position instead
                return QString("Fix %1 (%2)").arg(QString::fromStdString(pos->ident()))
                        .arg(formatGeodAsString(pos->geod()));
            } else {
                QString name = fixNavaidName(QString::fromStdString(pos->name()));
                return QString("%1: %2").arg(QString::fromStdString(pos->ident())).arg(name);
            }
        }

        if (role == Qt::EditRole) {
            return QString::fromStdString(pos->ident());
        }

        if (role == Qt::UserRole) {
            return static_cast<qlonglong>(m_ids[index.row()]);
        }

        return QVariant();
    }

    FGPositionedRef itemAtRow(unsigned int row) const
    {
        FGPositionedRef pos = m_items[row];
        if (!pos.valid()) {
            pos = NavDataCache::instance()->loadById(m_ids[row]);
            m_items[row] = pos;
        }

        return pos;
    }
Q_SIGNALS:
    void searchComplete();

private:


    void onSearchResultsPoll()
    {
        PositionedIDVec newIds = m_search->results();

        beginInsertRows(QModelIndex(), m_ids.size(), newIds.size() - 1);
        for (unsigned int i=m_ids.size(); i < newIds.size(); ++i) {
            m_ids.push_back(newIds[i]);
            m_items.push_back(FGPositionedRef()); // null ref
        }
        endInsertRows();

        if (m_search->isComplete()) {
            m_searchActive = false;
            m_search.reset();
            emit searchComplete();
        } else {
            QTimer::singleShot(100, this, &NavSearchModel::onSearchResultsPoll);
        }
    }

private:
    PositionedIDVec m_ids;
    mutable FGPositionedList m_items;
    bool m_searchActive;
    QScopedPointer<NavDataCache::ThreadedGUISearch> m_search;
};


LocationWidget::LocationWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::LocationWidget)
{
    m_ui->setupUi(this);


    QIcon historyIcon(":/history-icon");
    m_ui->searchHistory->setIcon(historyIcon);

    m_ui->searchIcon->setPixmap(QPixmap(":/search-icon"));

    m_searchModel = new NavSearchModel;
    m_ui->searchResultsList->setModel(m_searchModel);
    connect(m_ui->searchResultsList, &QListView::clicked,
            this, &LocationWidget::onSearchResultSelected);
    connect(m_searchModel, &NavSearchModel::searchComplete,
            this, &LocationWidget::onSearchComplete);

    connect(m_ui->runwayCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateDescription()));
    connect(m_ui->parkingCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateDescription()));
    connect(m_ui->runwayRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateDescription()));
    connect(m_ui->parkingRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateDescription()));
    connect(m_ui->onFinalCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateDescription()));
    connect(m_ui->approachDistanceSpin, SIGNAL(valueChanged(int)),
            this, SLOT(updateDescription()));

    connect(m_ui->airportDiagram, &AirportDiagram::clickedRunway,
            this, &LocationWidget::onAirportDiagramClicked);

    connect(m_ui->locationSearchEdit, &QLineEdit::returnPressed,
            this, &LocationWidget::onSearch);

    connect(m_ui->searchHistory, &QPushButton::clicked,
            this, &LocationWidget::onPopupHistory);

    connect(m_ui->trueBearing, &QCheckBox::toggled,
            this, &LocationWidget::onOffsetBearingTrueChanged);
    connect(m_ui->offsetGroup, &QGroupBox::toggled,
            this, &LocationWidget::onOffsetEnabledToggled);
    connect(m_ui->trueBearing, &QCheckBox::toggled, this,
            &LocationWidget::onOffsetDataChanged);
    connect(m_ui->offsetBearingSpinbox, SIGNAL(valueChanged(int)),
            this, SLOT(onOffsetDataChanged()));
    connect(m_ui->offsetNmSpinbox, SIGNAL(valueChanged(double)),
            this, SLOT(onOffsetDataChanged()));

    m_backButton = new QToolButton(this);
    m_backButton->setGeometry(0, 0, 32, 32);
    m_backButton->setIcon(QIcon(":/search-icon"));
    m_backButton->raise();

    connect(m_backButton, &QAbstractButton::clicked,
            this, &LocationWidget::onBackToSearch);

// force various pieces of UI into sync
    onOffsetEnabledToggled(m_ui->offsetGroup->isChecked());
    onOffsetBearingTrueChanged(m_ui->trueBearing->isChecked());
    onBackToSearch();
}

LocationWidget::~LocationWidget()
{
    delete m_ui;
}

void LocationWidget::restoreSettings()
{
    QSettings settings;
    Q_FOREACH(QVariant v, settings.value("recent-locations").toList()) {
        m_recentAirports.push_back(v.toLongLong());
    }

    if (!m_recentAirports.empty()) {
        setBaseLocation(NavDataCache::instance()->loadById(m_recentAirports.front()));
    }

    updateDescription();
}

bool LocationWidget::shouldStartPaused() const
{
    if (!m_location) {
        return false; // defaults to on-ground at KSFO
    }

    if (FGAirport::isAirportType(m_location.ptr())) {
        return m_ui->onFinalCheckbox->isChecked();
    } else {
        // navaid, start paused
        return true;
    }

    return false;
}

void LocationWidget::saveSettings()
{
    QSettings settings;

    QVariantList locations;
    Q_FOREACH(PositionedID v, m_recentAirports) {
        locations.push_back(v);
    }

    settings.setValue("recent-airports", locations);
}

void LocationWidget::setLocationOptions()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();

    if (!m_location) {
        return;
    }

    if (FGAirport::isAirportType(m_location.ptr())) {
        FGAirport* apt = static_cast<FGAirport*>(m_location.ptr());
        opt->addOption("airport", apt->ident());

        if (m_ui->runwayRadio->isChecked()) {
            int index = m_ui->runwayCombo->itemData(m_ui->runwayCombo->currentIndex()).toInt();
            if (index >= 0) {
                // explicit runway choice
                opt->addOption("runway", apt->getRunwayByIndex(index)->ident());
            }

            if (m_ui->onFinalCheckbox->isChecked()) {
                opt->addOption("glideslope", "3.0");
                opt->addOption("offset-distance", "10.0"); // in nautical miles
            }
        } else if (m_ui->parkingRadio->isChecked()) {
            // parking selection
            opt->addOption("parkpos", m_ui->parkingCombo->currentText().toStdString());
        }
        // of location is an airport
    }

    FGPositioned::Type ty = m_location->type();
    switch (ty) {
    case FGPositioned::VOR:
    case FGPositioned::NDB:
    case FGPositioned::FIX:
        // set disambiguation property
        globals->get_props()->setIntValue("/sim/presets/navaid-id",
                                          static_cast<int>(m_location->guid()));

        // we always set 'fix', but really this is just to force positionInit
        // code to check for the navaid-id value above.
        opt->addOption("fix", m_location->ident());
        break;
    default:
        break;
    }
}

void LocationWidget::onSearch()
{
    QString search = m_ui->locationSearchEdit->text();
    m_searchModel->setSearch(search);

    if (m_searchModel->isSearchActive()) {
        m_ui->searchStatusText->setText(QString("Searching for '%1'").arg(search));
        m_ui->searchIcon->setVisible(true);
    } else if (m_searchModel->rowCount(QModelIndex()) == 1) {
        setBaseLocation(m_searchModel->itemAtRow(0));
    }
}

void LocationWidget::onSearchComplete()
{
    QString search = m_ui->locationSearchEdit->text();
    m_ui->searchIcon->setVisible(false);
    m_ui->searchStatusText->setText(QString("Results for '%1'").arg(search));

    int numResults = m_searchModel->rowCount(QModelIndex());
    if (numResults == 0) {
        m_ui->searchStatusText->setText(QString("No matches for '%1'").arg(search));
    } else if (numResults == 1) {
        setBaseLocation(m_searchModel->itemAtRow(0));
    }
}

void LocationWidget::onLocationChanged()
{
    bool locIsAirport = FGAirport::isAirportType(m_location.ptr());
    m_backButton->show();

    if (locIsAirport) {
        m_ui->stack->setCurrentIndex(0);
        FGAirport* apt = static_cast<FGAirport*>(m_location.ptr());
        m_ui->airportDiagram->setAirport(apt);

        m_ui->runwayRadio->setChecked(true); // default back to runway mode
        // unless multiplayer is enabled ?
        m_ui->airportDiagram->setEnabled(true);

        m_ui->runwayCombo->clear();
        m_ui->runwayCombo->addItem("Automatic", -1);
        for (unsigned int r=0; r<apt->numRunways(); ++r) {
            FGRunwayRef rwy = apt->getRunwayByIndex(r);
            // add runway with index as data role
            m_ui->runwayCombo->addItem(QString::fromStdString(rwy->ident()), r);

            m_ui->airportDiagram->addRunway(rwy);
        }

        m_ui->parkingCombo->clear();
        FGAirportDynamics* dynamics = apt->getDynamics();
        PositionedIDVec parkings = NavDataCache::instance()->airportItemsOfType(m_location->guid(),
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


    } else {// of location is airport
        // navaid
        m_ui->stack->setCurrentIndex(1);
        m_ui->navaidDiagram->setNavaid(m_location);
    }
}

void LocationWidget::onOffsetEnabledToggled(bool on)
{
    m_ui->offsetDistanceLabel->setEnabled(on);
}

void LocationWidget::onAirportDiagramClicked(FGRunwayRef rwy)
{
    if (rwy) {
        m_ui->runwayRadio->setChecked(true);
        int rwyIndex = m_ui->runwayCombo->findText(QString::fromStdString(rwy->ident()));
        m_ui->runwayCombo->setCurrentIndex(rwyIndex);
        m_ui->airportDiagram->setSelectedRunway(rwy);
    }

    updateDescription();
}

QString LocationWidget::locationDescription() const
{
    if (!m_location)
        return QString("No location selected");

    bool locIsAirport = FGAirport::isAirportType(m_location.ptr());
    QString ident = QString::fromStdString(m_location->ident()),
        name = QString::fromStdString(m_location->name());

    name = fixNavaidName(name);

    if (locIsAirport) {
        FGAirport* apt = static_cast<FGAirport*>(m_location.ptr());
        QString locationOnAirport;

        if (m_ui->runwayRadio->isChecked()) {
            bool onFinal = m_ui->onFinalCheckbox->isChecked();
            int comboIndex = m_ui->runwayCombo->currentIndex();
            QString runwayName = (comboIndex == 0) ?
                "active runway" :
                QString("runway %1").arg(m_ui->runwayCombo->currentText());

            if (onFinal) {
                int finalDistance = m_ui->approachDistanceSpin->value();
                locationOnAirport = QString("on %2-mile final to %1").arg(runwayName).arg(finalDistance);
            } else {
                locationOnAirport = QString("on %1").arg(runwayName);
            }
        } else if (m_ui->parkingRadio->isChecked()) {
            locationOnAirport = QString("at parking position %1").arg(m_ui->parkingCombo->currentText());
        }

        return QString("%2 (%1): %3").arg(ident).arg(name).arg(locationOnAirport);
    } else {
        QString navaidType;
        switch (m_location->type()) {
        case FGPositioned::VOR:
            navaidType = QString("VOR"); break;
        case FGPositioned::NDB:
            navaidType = QString("NDB"); break;
        case FGPositioned::FIX:
            return QString("at waypoint %1").arg(ident);
        default:
            // unsupported type
            break;
        }

        return QString("at %1 %2 (%3)").arg(navaidType).arg(ident).arg(name);
    }

    return QString("Implement Me");
}


void LocationWidget::updateDescription()
{
    bool locIsAirport = FGAirport::isAirportType(m_location.ptr());
    if (locIsAirport) {
        FGAirport* apt = static_cast<FGAirport*>(m_location.ptr());

        if (m_ui->runwayRadio->isChecked()) {
            int comboIndex = m_ui->runwayCombo->currentIndex();
            int runwayIndex = m_ui->runwayCombo->itemData(comboIndex).toInt();
            // we can't figure out the active runway in the launcher (yet)
            FGRunwayRef rwy = (runwayIndex >= 0) ?
                apt->getRunwayByIndex(runwayIndex) : FGRunwayRef();
            m_ui->airportDiagram->setSelectedRunway(rwy);
        }

        if (m_ui->onFinalCheckbox->isChecked()) {
            m_ui->airportDiagram->setApproachExtensionDistance(m_ui->approachDistanceSpin->value());
        } else {
            m_ui->airportDiagram->setApproachExtensionDistance(0.0);
        }
    } else {

    }

#if 0

    QString locationOnAirport;
    if (m_ui->runwayRadio->isChecked()) {


    } else if (m_ui->parkingRadio->isChecked()) {
        locationOnAirport =  QString("at parking position %1").arg(m_ui->parkingCombo->currentText());
    }

    m_ui->airportDescription->setText();
#endif

    emit descriptionChanged(locationDescription());
}

void LocationWidget::onSearchResultSelected(const QModelIndex& index)
{
    qDebug() << "selected result:" << index.data();
    setBaseLocation(m_searchModel->itemAtRow(index.row()));
}

void LocationWidget::onOffsetBearingTrueChanged(bool on)
{
    m_ui->offsetBearingLabel->setText(on ? tr("True bearing:") :
                                      tr("Magnetic bearing:"));
}


void LocationWidget::onPopupHistory()
{
    if (m_recentAirports.isEmpty()) {
        return;
    }

#if 0
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
#endif
}

void LocationWidget::setBaseLocation(FGPositionedRef ref)
{
    if (m_location == ref)
        return;

    m_location = ref;
    onLocationChanged();

#if 0
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
#endif
    updateDescription();
}

void LocationWidget::onOffsetDataChanged()
{
    qDebug() << "implement me";
}

void LocationWidget::onBackToSearch()
{
    m_ui->stack->setCurrentIndex(2);
    m_backButton->hide();
}

#include "LocationWidget.moc"
