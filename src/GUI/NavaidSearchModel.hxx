// NavaidSearchModel.hxx - expose navaids via a QabstractListModel
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

#ifndef NAVAIDSEARCHMODEL_HXX
#define NAVAIDSEARCHMODEL_HXX

#include <QAbstractListModel>

#include "LauncherController.hxx"

#include <Navaids/positioned.hxx>
#include <Navaids/NavDataCache.hxx>

class NavaidSearchModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchActiveChanged)
    Q_PROPERTY(bool haveExistingSearch READ haveExistingSearch NOTIFY haveExistingSearchChanged)
    Q_PROPERTY(bool airportsOnly MEMBER m_airportsOnly NOTIFY airportsOnlyChanged)
    Q_PROPERTY(int maxResults MEMBER m_maxResults NOTIFY maxResultsChanged)
    Q_PROPERTY(int numResults READ numResults NOTIFY searchActiveChanged)

    Q_PROPERTY(qlonglong exactMatch READ exactMatch NOTIFY searchActiveChanged)

    enum Roles {
        GeodRole = Qt::UserRole + 1,
        GuidRole = Qt::UserRole + 2,
        IdentRole = Qt::UserRole + 3,
        NameRole = Qt::UserRole + 4,
        IconRole = Qt::UserRole + 5,
        TypeRole = Qt::UserRole + 6,
        NavFrequencyRole = Qt::UserRole + 7
    };

public:
    NavaidSearchModel(QObject* parent = nullptr);

    enum AircraftType
    {
        Unknown = LauncherController::Unknown,
        Airplane = LauncherController::Airplane,
        Seaplane = LauncherController::Seaplane,
        Helicopter = LauncherController::Helicopter,
        Airship = LauncherController::Airship
    };

    Q_ENUMS(AircraftType)

    Q_INVOKABLE void setSearch(QString t, AircraftType aircraft = Unknown);

    Q_INVOKABLE void clear();

    Q_INVOKABLE qlonglong guidAtIndex(int index) const;

    bool isSearchActive() const
    {
        return m_searchActive;
    }

    bool haveExistingSearch() const;

    int rowCount(const QModelIndex&) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    FGPositionedRef itemAtRow(unsigned int row) const;

    void setItems(const FGPositionedList& items);

    QHash<int, QByteArray> roleNames() const override;

    qlonglong exactMatch() const;

    int numResults() const;
Q_SIGNALS:
    void searchComplete();
    void searchActiveChanged();
    void haveExistingSearchChanged();
    void airportsOnlyChanged();
    void maxResultsChanged();

private slots:
    void onSearchResultsPoll();

private:
    void resort();

    PositionedIDVec m_ids;
    mutable FGPositionedList m_items;
    bool m_searchActive = false;
    bool m_airportsOnly = false;
    int m_maxResults = 0;
    QScopedPointer<flightgear::NavDataCache::ThreadedGUISearch> m_search;
};


#endif // NAVAIDSEARCHMODEL_HXX
