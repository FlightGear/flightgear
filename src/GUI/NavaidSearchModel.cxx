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

        if (aircraft == LauncherController::Helicopter) {
            addType(FGPositioned::HELIPAD);
        }

        if (aircraft == LauncherController::Seaplane) {
            addType(FGPositioned::SEAPORT);
        } else {
            addType(FGPositioned::AIRPORT);
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

void NavaidSearchModel::setSearch(QString t, NavaidSearchModel::AircraftType aircraft)
{
    beginResetModel();

    m_items.clear();
    m_ids.clear();

    std::string term(t.toUpper().toStdString());

    IdentSearchFilter filter(static_cast<LauncherController::AircraftType>(aircraft), m_airportsOnly);
    FGPositionedList exactMatches = NavDataCache::instance()->findAllWithIdent(term, &filter, true);

    // truncate based on max results
    if ((m_maxResults > 0) && (exactMatches.size() > m_maxResults)) {
        auto it = exactMatches.begin() + m_maxResults;
        exactMatches.erase(it, exactMatches.end());
    }

    m_ids.reserve(exactMatches.size());
    m_items.reserve(exactMatches.size());
    for (auto match : exactMatches) {
        m_ids.push_back(match->guid());
        m_items.push_back(match);
    }
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
    return m_ids.size();
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

void NavaidSearchModel::onSearchResultsPoll()
{
    if (m_search.isNull()) {
        return;
    }

    PositionedIDVec newIds = m_search->results();
    int newTotalSize = m_ids.size() + newIds.size();
    if ((m_maxResults > 0) && (newTotalSize > m_maxResults)) {
        // truncate new results as necessary
        int numNewAllowed = m_maxResults - m_ids.size();
        auto it = newIds.begin() + numNewAllowed;
        newIds.erase(it, newIds.end());
        // possible that newIDs is empty now
    }

    if (!newIds.empty()) {
        beginInsertRows(QModelIndex(), m_ids.size(), newIds.size() - 1);
        for (auto id : newIds) {
            m_ids.push_back(id);
            m_items.push_back({}); // null ref
        }
        endInsertRows();
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
