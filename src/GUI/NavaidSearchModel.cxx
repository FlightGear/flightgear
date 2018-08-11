// NavaidSearchModel.cxx - expose navaids via a QabstractListModel
//
// Written by James Turner, started July 2018.
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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

#include "NavaidSearchModel.hxx"

#include <QTimer>

#include "AirportDiagram.hxx"
#include <Navaids/navrecord.hxx>
#include "QmlPositioned.hxx"

using namespace flightgear;

QString fixNavaidName(QString s)
{
    // split into words
    QStringList words = s.split(QChar(' '));
    QStringList changedWords;
    Q_FOREACH(QString w, words) {
        QString up = w.toUpper();

        // expand common abbreviations
        // note these are not translated, since they are abbreivations
        // for English-langauge airports, mostly in the US/Canada
        if (up == "FLD") {
            changedWords.append("Field");
            continue;
        }

        if (up == "CO") {
            changedWords.append("County");
            continue;
        }

        if ((up == "MUNI") || (up == "MUN")) {
            changedWords.append("Municipal");
            continue;
        }

        if (up == "MEM") {
            changedWords.append("Memorial");
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

        if ((up == "VOR") || (up == "NDB")
                || (up == "VOR-DME") || (up == "VORTAC")
                || (up == "NDB-DME")
                || (up == "AFB") || (up == "RAF"))
        {
            changedWords.append(w);
            continue;
        }

        if ((up =="[X]") || (up == "[H]") || (up == "[S]")) {
            continue; // consume
        }

        QChar firstChar = w.at(0).toUpper();
        w = w.mid(1).toLower();
        w.prepend(firstChar);

        changedWords.append(w);
    }

    return changedWords.join(QChar(' '));
}


class IdentSearchFilter : public FGPositioned::TypeFilter
{
public:
    IdentSearchFilter(LauncherController::AircraftType aircraft, bool airportsOnly)
    {
        if (!airportsOnly) {
            addType(FGPositioned::VOR);
            addType(FGPositioned::FIX);
            addType(FGPositioned::NDB);
        }

        addType(FGPositioned::AIRPORT);

        switch (aircraft) {
        case LauncherController::Airplane:
            break;
                
        case LauncherController::Helicopter:
            addType(FGPositioned::HELIPORT);
            break;
                
        case LauncherController::Seaplane:
            addType(FGPositioned::SEAPORT);
            break;
                
        default:
            addType(FGPositioned::HELIPORT);
            addType(FGPositioned::SEAPORT);
        }
    }
};

void NavaidSearchModel::clear()
{
    beginResetModel();
    m_items.clear();
    m_ids.clear();
    m_searchActive = false;
    m_search.reset();
    endResetModel();
    emit searchActiveChanged();
    emit haveExistingSearchChanged();
}

qlonglong NavaidSearchModel::guidAtIndex(int index) const
{
    if ((index < 0) || (index >= m_ids.size()))
        return 0;

    return m_ids.at(index);
}

void NavaidSearchModel::setSearch(QString t, NavaidSearchModel::AircraftType aircraft)
{
    beginResetModel();

    m_items.clear();
    m_ids.clear();

    std::string term(t.toUpper().toStdString());

    IdentSearchFilter filter(static_cast<LauncherController::AircraftType>(aircraft), m_airportsOnly);
    FGPositionedList exactMatches = NavDataCache::instance()->findAllWithIdent(term, &filter, true);

    m_ids.reserve(exactMatches.size());
    m_items.reserve(exactMatches.size());
    for (auto match : exactMatches) {
        m_ids.push_back(match->guid());
        m_items.push_back(match);
    }

    resort();
    endResetModel();

    m_search.reset(new NavDataCache::ThreadedGUISearch(term, m_airportsOnly));
    QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
    m_searchActive = true;
    emit searchActiveChanged();
    emit haveExistingSearchChanged();
}

bool NavaidSearchModel::haveExistingSearch() const
{
    return m_searchActive || (!m_items.empty());
}

int NavaidSearchModel::rowCount(const QModelIndex &) const
{
    if (m_maxResults > 0)
        return std::min(static_cast<int>(m_ids.size()), m_maxResults);

    return static_cast<int>(m_ids.size());
}

QVariant NavaidSearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FGPositionedRef pos = itemAtRow(index.row());
    switch (role) {
    case GuidRole: return static_cast<qlonglong>(pos->guid());
    case IdentRole: return QString::fromStdString(pos->ident());
    case NameRole:
        return fixNavaidName(QString::fromStdString(pos->name()));

    case NavFrequencyRole: {
        FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(pos);
        return nav ? nav->get_freq() : 0;
    }

    case TypeRole: return static_cast<QmlPositioned::Type>(pos->type());
    case IconRole:
        return AirportDiagram::iconForPositioned(pos,
                                                 AirportDiagram::SmallIcons | AirportDiagram::LargeAirportPlans);
    }

    return {};
}

FGPositionedRef NavaidSearchModel::itemAtRow(unsigned int row) const
{
    FGPositionedRef pos = m_items[row];
    if (!pos.valid()) {
        pos = NavDataCache::instance()->loadById(m_ids[row]);
        m_items[row] = pos;
    }

    return pos;
}

void NavaidSearchModel::setItems(const FGPositionedList &items)
{
    beginResetModel();
    m_searchActive = false;
    m_items = items;

    m_ids.clear();
    for (unsigned int i=0; i < items.size(); ++i) {
        m_ids.push_back(m_items[i]->guid());
    }

    // don't sort in this case
    endResetModel();
    emit searchActiveChanged();
}

QHash<int, QByteArray> NavaidSearchModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();

    result[GeodRole] = "geod";
    result[GuidRole] = "guid";
    result[IdentRole] = "ident";
    result[NameRole] = "name";
    result[IconRole] = "icon";
    result[TypeRole] = "type";
    result[NavFrequencyRole] = "frequency";
    return result;
}

qlonglong NavaidSearchModel::exactMatch() const
{
    if (m_searchActive || (m_ids.size() != 1))
        return 0; // no exact match

    return m_ids.back(); // which is also the front
}

int NavaidSearchModel::numResults() const
{
    return static_cast<int>(m_ids.size());
}

void NavaidSearchModel::onSearchResultsPoll()
{
    if (m_search.isNull()) {
        return;
    }

    PositionedIDVec newIds = m_search->results();
    if (!newIds.empty()) {
        beginResetModel(); // reset the model since we will re-sort
        for (auto id : newIds) {
            m_ids.push_back(id);
            m_items.push_back({}); // null ref
        }
        resort();
        endResetModel();
    }

    if (m_search->isComplete()) {
        m_searchActive = false;
        m_search.reset();
        emit searchComplete();
        emit searchActiveChanged();
        emit haveExistingSearchChanged();
    } else {
        QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
    }
}

void NavaidSearchModel::resort()
{
    if (!m_airportsOnly) {
        return;
    }

    // clear m_items
    std::fill(m_items.begin(), m_items.end(), FGPositionedRef{});

    // build runway length cache
    std::map<PositionedID, double> longestRunwayCache;
    for (auto a : m_ids) {
        const FGAirportRef apt = FGPositioned::loadById<FGAirport>(a);
        if (apt) {
            const auto rwy = apt->longestRunway();
            if (rwy) {
                longestRunwayCache[a] = rwy->lengthFt();
            }
        }
    }

    std::sort(m_ids.begin(), m_ids.end(),
              [&longestRunwayCache](const PositionedID a, const PositionedID& b)
    {
        return longestRunwayCache[a] > longestRunwayCache[b];
    });
}
