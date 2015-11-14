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
#include <QMovie>
#include <QPainter>

#include "AirportDiagram.hxx"
#include "NavaidDiagram.hxx"

#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx> // for parking
#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx> // for fgSetDouble

using namespace flightgear;

const unsigned int MAX_RECENT_LOCATIONS = 64;

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

bool parseStringAsGeod(const QString& s, SGGeod& result)
{
    int commaPos = s.indexOf(QChar(','));
    if (commaPos < 0)
        return false;

    bool ok;
    double lon = s.leftRef(commaPos).toDouble(&ok);
    if (!ok)
        return false;

    double lat = s.midRef(commaPos+1).toDouble(&ok);
    if (!ok)
        return false;

    result = SGGeod::fromDeg(lon, lat);
    return true;
}

QVariant savePositionList(const FGPositionedList& posList)
{
    QVariantList vl;
    FGPositionedList::const_iterator it;
    for (it = posList.begin(); it != posList.end(); ++it) {
        QVariantMap vm;
        FGPositionedRef pos = *it;
        vm.insert("ident", QString::fromStdString(pos->ident()));
        vm.insert("type", pos->type());
        vm.insert("lat", pos->geod().getLatitudeDeg());
        vm.insert("lon", pos->geod().getLongitudeDeg());
        vl.append(vm);
    }
    return vl;
}

FGPositionedList loadPositionedList(QVariant v)
{
    QVariantList vl = v.toList();
    FGPositionedList result;
    result.reserve(vl.size());
    NavDataCache* cache = NavDataCache::instance();

    Q_FOREACH(QVariant v, vl) {
        QVariantMap vm = v.toMap();
        std::string ident(vm.value("ident").toString().toStdString());
        double lat = vm.value("lat").toDouble();
        double lon = vm.value("lon").toDouble();
        FGPositioned::Type ty(static_cast<FGPositioned::Type>(vm.value("type").toInt()));
        FGPositioned::TypeFilter filter(ty);
        FGPositionedRef pos = cache->findClosestWithIdent(ident,
                                                          SGGeod::fromDeg(lon, lat),
                                                          &filter);
        if (pos)
            result.push_back(pos);
    }

    return result;
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

        if (role == Qt::DecorationRole) {
            return AirportDiagram::iconForPositioned(pos, true);
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

    void setItems(const FGPositionedList& items)
    {
        beginResetModel();
        m_searchActive = false;
        m_items = items;

        m_ids.clear();
        for (unsigned int i=0; i < items.size(); ++i) {
            m_ids.push_back(m_items[i]->guid());
        }

        endResetModel();
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

    QByteArray format;
    m_ui->searchIcon->setMovie(new QMovie(":/spinner", format, this));

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
            this, &LocationWidget::onShowHistory);

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
    connect(m_ui->headingSpinbox, SIGNAL(valueChanged(int)),
            this, SLOT(onHeadingChanged()));

    m_backButton = new QToolButton(this);
    m_backButton->setGeometry(0, 0, 64, 32);
    m_backButton->setText("<< Back");
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

    if (settings.contains("location-lat")) {
        m_locationIsLatLon = true;
        m_geodLocation = SGGeod::fromDeg(settings.value("location-lon").toDouble(),
                                         settings.value("location-lat").toDouble());
    } else if (settings.contains("location-id")) {
        m_location = NavDataCache::instance()->loadById(settings.value("location-id").toULongLong());
    }

    m_ui->altitudeSpinbox->setValue(settings.value("altitude", 6000).toInt());
    m_ui->airspeedSpinbox->setValue(settings.value("speed", 120).toInt());
    m_ui->headingSpinbox->setValue(settings.value("heading").toInt());
    m_ui->offsetGroup->setChecked(settings.value("offset-enabled").toBool());
    m_ui->offsetBearingSpinbox->setValue(settings.value("offset-bearing").toInt());
    m_ui->offsetNmSpinbox->setValue(settings.value("offset-distance", 10).toInt());

    m_recentLocations = loadPositionedList(settings.value("recent-locations"));
    m_searchModel->setItems(m_recentLocations);

    onLocationChanged();
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

    settings.remove("location-id");
    if (m_locationIsLatLon) {
        settings.setValue("location-lat", m_geodLocation.getLatitudeDeg());
        settings.setValue("location-lon", m_geodLocation.getLongitudeDeg());

    } else if (m_location) {
        settings.setValue("location-id", static_cast<qlonglong>(m_location->guid()));
    }

    settings.setValue("altitude", m_ui->altitudeSpinbox->value());
    settings.setValue("speed", m_ui->airspeedSpinbox->value());

    settings.setValue("offset-enabled", m_ui->offsetGroup->isChecked());
    settings.setValue("offset-bearing", m_ui->offsetBearingSpinbox->value());
    settings.setValue("offset-distance", m_ui->offsetNmSpinbox->value());

    // recent locations is saved on modification
}

void LocationWidget::setLocationOptions()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();

    std::string altStr = QString::number(m_ui->altitudeSpinbox->value()).toStdString();
    std::string vcStr = QString::number(m_ui->airspeedSpinbox->value()).toStdString();
    std::string headingStr = QString::number(m_ui->headingSpinbox->value()).toStdString();

    // flip direction of azimuth to balance the flip done in fgApplyStartOffset
    // I don't know why that flip exists but changing it there will break
    // command-line compatability so compensating here instead
    int offsetAzimuth = m_ui->offsetBearingSpinbox->value() - 180;
    std::string azimuthStr = QString::number(offsetAzimuth).toStdString();
    std::string distanceStr = QString::number(m_ui->offsetNmSpinbox->value()).toStdString();

    if (m_locationIsLatLon) {
        // bypass the options mechanism because converting to deg:min:sec notation
        // just to parse back again is nasty.
        fgSetDouble("/sim/presets/latitude-deg", m_geodLocation.getLatitudeDeg());
        fgSetDouble("/position/latitude-deg", m_geodLocation.getLatitudeDeg());
        fgSetDouble("/sim/presets/longitude-deg", m_geodLocation.getLongitudeDeg());
        fgSetDouble("/position/longitude-deg", m_geodLocation.getLongitudeDeg());

        opt->addOption("altitude", altStr);
        opt->addOption("vc", vcStr);
        opt->addOption("heading", headingStr);

        if (m_ui->offsetGroup->isChecked()) {
            opt->addOption("offset-azimuth", azimuthStr);
            opt->addOption("offset-distance", distanceStr);
        }
        return;
    }

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
                FGRunwayRef runway = apt->getRunwayByIndex(index);
                opt->addOption("runway", runway->ident());

                // set nav-radio 1 based on selected runway
                if (runway->ILS()) {
                    double mhz = runway->ILS()->get_freq() / 100.0;
                    QString navOpt = QString("%1:%2").arg(runway->headingDeg()).arg(mhz);
                    opt->addOption("nav1", navOpt.toStdString());
                }
            }

            if (m_ui->onFinalCheckbox->isChecked()) {
                opt->addOption("glideslope", "3.0");
                double offsetNm = m_ui->approachDistanceSpin->value();
                opt->addOption("offset-distance", QString::number(offsetNm).toStdString());
            }

        } else if (m_ui->parkingRadio->isChecked()) {
            // parking selection
            opt->addOption("parkpos", m_ui->parkingCombo->currentText().toStdString());
        }
        // of location is an airport
    } else {
        // location is a navaid
        // note setting the ident here is ambigious, we really only need and
        // want the 'navaid-id' property. However setting the 'real' option
        // gives a better UI experience (eg existing Position in Air dialog)
        FGPositioned::Type ty = m_location->type();
        switch (ty) {
        case FGPositioned::VOR:
            opt->addOption("vor", m_location->ident());
            setNavRadioOption();
            break;

        case FGPositioned::NDB:
            opt->addOption("ndb", m_location->ident());
            setNavRadioOption();
            break;

        case FGPositioned::FIX:
            opt->addOption("fix", m_location->ident());
            break;
        default:
            break;
        }

        opt->addOption("altitude", altStr);
        opt->addOption("vc", vcStr);
        opt->addOption("heading", headingStr);

        // set disambiguation property
        globals->get_props()->setIntValue("/sim/presets/navaid-id",
                                          static_cast<int>(m_location->guid()));

        if (m_ui->offsetGroup->isChecked()) {
            opt->addOption("offset-azimuth", azimuthStr);
            opt->addOption("offset-distance", distanceStr);
        }
    } // of navaid location
}

void LocationWidget::setNavRadioOption()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();

    if (m_location->type() == FGPositioned::VOR) {
        FGNavRecordRef nav(static_cast<FGNavRecord*>(m_location.ptr()));
        double mhz = nav->get_freq() / 100.0;
        int heading = 0; // add heading support
        QString navOpt = QString("%1:%2").arg(heading).arg(mhz);
        opt->addOption("nav1", navOpt.toStdString());
    } else {
        FGNavRecordRef nav(static_cast<FGNavRecord*>(m_location.ptr()));
        int khz = nav->get_freq() / 100;
        int heading = 0;
        QString adfOpt = QString("%1:%2").arg(heading).arg(khz);
        qDebug() << "ADF opt is:" << adfOpt;
        opt->addOption("adf1", adfOpt.toStdString());
    }
}

void LocationWidget::onSearch()
{
    QString search = m_ui->locationSearchEdit->text();

    m_locationIsLatLon = parseStringAsGeod(search, m_geodLocation);
    if (m_locationIsLatLon) {
        m_ui->searchIcon->setVisible(false);
        m_ui->searchStatusText->setText(QString("Position '%1'").arg(formatGeodAsString(m_geodLocation)));
        m_location.clear();
        onLocationChanged();
        updateDescription();
        return;
    }

    m_searchModel->setSearch(search);

    if (m_searchModel->isSearchActive()) {
        m_ui->searchStatusText->setText(QString("Searching for '%1'").arg(search));
        qDebug() << "setting icon visible";
        m_ui->searchIcon->setVisible(true);
        m_ui->searchIcon->movie()->start();
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
        addToRecent(m_searchModel->itemAtRow(0));
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

    } else if (m_locationIsLatLon) {
        m_ui->stack->setCurrentIndex(1);
        m_ui->navaidDiagram->setGeod(m_geodLocation);
    } else if (m_location) {
        // navaid
        m_ui->stack->setCurrentIndex(1);
        m_ui->navaidDiagram->setNavaid(m_location);
    }
}

void LocationWidget::onOffsetEnabledToggled(bool on)
{
    m_ui->navaidDiagram->setOffsetEnabled(on);
    updateDescription();
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

QString compassPointFromHeading(int heading)
{
    const int labelArc = 360 / 8;
    heading += (labelArc >> 1);
    SG_NORMALIZE_RANGE(heading, 0, 359);

    switch (heading / labelArc) {
    case 0: return "N";
    case 1: return "NE";
    case 2: return "E";
    case 3: return "SE";
    case 4: return "S";
    case 5: return "SW";
    case 6: return "W";
    case 7: return "NW";
    }

    return QString();
}

QString LocationWidget::locationDescription() const
{
    if (!m_location) {
        if (m_locationIsLatLon) {
            return QString("at position %1").arg(formatGeodAsString(m_geodLocation));
        }

        return QString("No location selected");
    }

    bool locIsAirport = FGAirport::isAirportType(m_location.ptr());
    QString ident = QString::fromStdString(m_location->ident()),
        name = QString::fromStdString(m_location->name());

    name = fixNavaidName(name);

    if (locIsAirport) {
        //FGAirport* apt = static_cast<FGAirport*>(m_location.ptr());
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
        QString offsetDesc = tr("at");
        if (m_ui->offsetGroup->isChecked()) {
            offsetDesc = QString("%1nm %2 of").
                    arg(m_ui->offsetNmSpinbox->value(), 0, 'f', 1).
                    arg(compassPointFromHeading(m_ui->offsetBearingSpinbox->value()));
        }

        QString navaidType;
        switch (m_location->type()) {
        case FGPositioned::VOR:
            navaidType = QString("VOR"); break;
        case FGPositioned::NDB:
            navaidType = QString("NDB"); break;
        case FGPositioned::FIX:
            return QString("%2 waypoint %1").arg(ident).arg(offsetDesc);
        default:
            // unsupported type
            break;
        }

        return QString("%4 %1 %2 (%3)").arg(navaidType).arg(ident).arg(name).arg(offsetDesc);
    }

    return tr("No location selected");
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
    FGPositionedRef pos = m_searchModel->itemAtRow(index.row());
    addToRecent(pos);
    setBaseLocation(pos);
}

void LocationWidget::onOffsetBearingTrueChanged(bool on)
{
    m_ui->offsetBearingLabel->setText(on ? tr("True bearing:") :
                                           tr("Magnetic bearing:"));
}

void LocationWidget::addToRecent(FGPositionedRef pos)
{
    FGPositionedList::iterator it = std::find(m_recentLocations.begin(),
                                              m_recentLocations.end(),
                                              pos);
    if (it != m_recentLocations.end()) {
        m_recentLocations.erase(it);
    }

    if (m_recentLocations.size() >= MAX_RECENT_LOCATIONS) {
        m_recentLocations.pop_back();
    }

    m_recentLocations.insert(m_recentLocations.begin(), pos);
    QSettings settings;
    settings.setValue("recent-locations", savePositionList(m_recentLocations));
}


void LocationWidget::onShowHistory()
{
    qDebug() << Q_FUNC_INFO;
    m_searchModel->setItems(m_recentLocations);
}

void LocationWidget::setBaseLocation(FGPositionedRef ref)
{
    if (m_location == ref)
        return;

    m_location = ref;
    onLocationChanged();

    updateDescription();
}

void LocationWidget::onOffsetDataChanged()
{
    m_ui->navaidDiagram->setOffsetEnabled(m_ui->offsetGroup->isChecked());
    m_ui->navaidDiagram->setOffsetBearingDeg(m_ui->offsetBearingSpinbox->value());
    m_ui->navaidDiagram->setOffsetDistanceNm(m_ui->offsetNmSpinbox->value());

    updateDescription();
}

void LocationWidget::onHeadingChanged()
{
    m_ui->navaidDiagram->setHeadingDeg(m_ui->headingSpinbox->value());
}

void LocationWidget::onBackToSearch()
{
    m_ui->stack->setCurrentIndex(2);
    m_backButton->hide();
}

#include "LocationWidget.moc"
