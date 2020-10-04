#include "AircraftProxyModel.hxx"

#include <QSettings>
#include <QDebug>

#include "AircraftItemModel.hxx"
#include "FavouriteAircraftData.hxx"

#include <simgear/package/Package.hxx>

AircraftProxyModel::AircraftProxyModel(QObject *pr, QAbstractItemModel * source) :
    QSortFilterProxyModel(pr)
{
    m_ratings = {3, 3, 3, 3};
    setSourceModel(source);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    // important we sort on the primary name role and not Qt::DisplayRole
    // otherwise the aircraft jump when switching variant
    setSortRole(AircraftVariantDescriptionRole);

    setDynamicSortFilter(true);

    // kick off initial sort
    sort(0);

    connect(this, &QAbstractItemModel::rowsInserted, this, &AircraftProxyModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &AircraftProxyModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &AircraftProxyModel::countChanged);
}

void AircraftProxyModel::setRatings(QList<int> ratings)
{
    if (ratings == m_ratings)
        return;
    m_ratings = ratings;
    invalidate();
    emit ratingsChanged();
    emit summaryTextChanged();
}

void AircraftProxyModel::setAircraftFilterString(QString s)
{
    m_filterString = s;

    m_filterProps = new SGPropertyNode;
    int index = 0;
    Q_FOREACH(QString term, s.split(QRegExp("\\W+"), QString::SkipEmptyParts)) {
        m_filterProps->getNode("all-of/text", index++, true)->setStringValue(term.toStdString());
    }

    invalidate();
    emit countChanged();
}

int AircraftProxyModel::indexForURI(QUrl uri) const
{
    auto sourceIndex = qobject_cast<AircraftItemModel*>(sourceModel())->indexOfAircraftURI(uri);
    auto ourIndex = mapFromSource(sourceIndex);
    if (!sourceIndex.isValid() || !ourIndex.isValid()) {
        return -1;
    }

    return ourIndex.row();
}

void AircraftProxyModel::selectVariantForAircraftURI(QUrl uri)
{
    qobject_cast<AircraftItemModel*>(sourceModel())->selectVariantForAircraftURI(uri);
}

void AircraftProxyModel::setRatingFilterEnabled(bool e)
{
    if (e == m_ratingsFilter) {
        return;
    }

    m_ratingsFilter = e;
    invalidate();
    emit ratingsFilterEnabledChanged();
    emit summaryTextChanged();
    emit countChanged();
}

QString AircraftProxyModel::summaryText() const
{
    const int unfilteredCount = sourceModel()->rowCount();
    if (m_ratingsFilter) {
        return tr("(%1 of %2 aircraft)").arg(rowCount()).arg(unfilteredCount);
    }

    return tr("(%1 aircraft)").arg(unfilteredCount);
}

int AircraftProxyModel::count() const
{
    return rowCount();
}

void AircraftProxyModel::setInstalledFilterEnabled(bool e)
{
    if (e == m_onlyShowInstalled) {
        return;
    }

    m_onlyShowInstalled = e;
    invalidate();
}

void AircraftProxyModel::setHaveUpdateFilterEnabled(bool e)
{
    if (e == m_onlyShowWithUpdate)
        return;

    m_onlyShowWithUpdate = e;
    invalidate();
}

void AircraftProxyModel::setShowFavourites(bool e)
{
    if (e == m_onlyShowFavourites)
        return;

    m_onlyShowFavourites = e;
    if (e) {
        setDynamicSortFilter(false);
        connect(FavouriteAircraftData::instance(), &FavouriteAircraftData::changed,
                [this]() {
            this->invalidate();
        });
    }
    invalidate();
}

bool AircraftProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QVariant v = index.data(AircraftPackageStatusRole);

    if (!filterAircraft(index)) {
        return false;
    }

    if (m_onlyShowInstalled) {
        QVariant v = index.data(AircraftPackageStatusRole);
        const auto status = static_cast<LocalAircraftCache::PackageStatus>(v.toInt());
        if (status == LocalAircraftCache::PackageNotInstalled) {
            return false;
        }
    }

    if (m_onlyShowWithUpdate) {
        QVariant v = index.data(AircraftPackageStatusRole);
        const auto status = static_cast<LocalAircraftCache::PackageStatus>(v.toInt());
        switch (status) {
        case LocalAircraftCache::PackageNotInstalled:
        case LocalAircraftCache::PackageInstalled:
        case LocalAircraftCache::NotPackaged:
            return false; // no updated need / possible

        // otherwise, show in the update list
        default:
            break;
        }
    }

    if (m_onlyShowFavourites) {
        if (!index.data(AircraftIsFavouriteRole).toBool())
            return false;
    }

    // if there is no search active, i.e we are browsing, we might apply the
    // ratings filter.
    if (m_filterString.isEmpty() && !m_onlyShowInstalled && m_ratingsFilter) {
        for (int i=0; i<m_ratings.size(); ++i) {
            if (m_ratings.at(i) > index.data(AircraftRatingRole + i).toInt()) {
                return false;
            }
        }
    }

    return true;
}

bool AircraftProxyModel::filterAircraft(const QModelIndex &sourceIndex) const
{
    if (m_filterString.isEmpty()) {
        return true;
    }

    simgear::pkg::PackageRef pkg = sourceIndex.data(AircraftPackageRefRole).value<simgear::pkg::PackageRef>();
    if (pkg) {
        return pkg->matches(m_filterProps.ptr());
    }

    QString baseName = sourceIndex.data(Qt::DisplayRole).toString();
    if (baseName.contains(m_filterString, Qt::CaseInsensitive)) {
        return true;
    }

    QString longDesc = sourceIndex.data(AircraftLongDescriptionRole).toString();
    if (longDesc.contains(m_filterString, Qt::CaseInsensitive)) {
        return true;
    }

    const int variantCount = sourceIndex.data(AircraftVariantCountRole).toInt();
    for (int variant = 0; variant < variantCount; ++variant) {
        QString desc = sourceIndex.data(AircraftVariantDescriptionRole + variant).toString();
        if (desc.contains(m_filterString, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

void AircraftProxyModel::loadRatingsSettings()
{
    QSettings settings;
    m_ratingsFilter = settings.value("enable-ratings-filter", true).toBool();
    QVariantList vRatings = settings.value("ratings-filter").toList();
    if (vRatings.size() == 4) {
        for (int i=0; i < 4; ++i) {
            m_ratings[i] = vRatings.at(i).toInt();
        }
    }

    invalidate();
}

void AircraftProxyModel::saveRatingsSettings()
{
    QSettings settings;
    settings.setValue("enable-ratings-filter", m_ratingsFilter);
    QVariantList vRatings;
    for (int i=0; i < 4; ++i) {
        vRatings.append(m_ratings.at(i));
    }
    settings.setValue("ratings-filter", vRatings);
}

///
/// Custom sorting based on aircraft variants and URI
///
/// \param left first item to sort
/// \param right second item to sort
/// \return 0 when the items are equal, < 0 or > 0 when they differs
bool AircraftProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const QString variantLeft = left.data(AircraftVariantDescriptionRole).toString();
    const QString variantRight = right.data(AircraftVariantDescriptionRole).toString();

    // we're comparing by default by variantDescriptionRole but when the variantDescriptionRole
    // is equal (e.g. two the same aircrafts installed from different sources - fgaddon + git)
    // we sort them by the AircraftURIRole. This ensures that the order of the same
    // items in the view is constant
    const int c = QString::compare(variantLeft, variantRight, Qt::CaseInsensitive);
    if (c == 0) {
        const QString uriLeft = left.data(AircraftURIRole).toString();
        const QString uriRight = right.data(AircraftURIRole).toString();
        return QString::localeAwareCompare(uriLeft, uriRight) < 0;
    } else {
        return c < 0;
    }
}
