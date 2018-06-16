#include "AircraftSearchFilterModel.hxx"

#include "AircraftModel.hxx"
#include <simgear/package/Package.hxx>

AircraftProxyModel::AircraftProxyModel(QObject *pr, QAbstractItemModel * source) :
    QSortFilterProxyModel(pr)
{
    m_ratings = {3, 3, 3, 3};
    setSourceModel(source);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(Qt::DisplayRole);
    setDynamicSortFilter(true);

    // kick off initial sort
    sort(0);
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
    Q_FOREACH(QString term, s.split(' ')) {
        m_filterProps->getNode("all-of/text", index++, true)->setStringValue(term.toStdString());
    }

    invalidate();
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
}

QString AircraftProxyModel::summaryText() const
{
    const int unfilteredCount = sourceModel()->rowCount();
    if (m_ratingsFilter) {
        return tr("(%1 of %2 aircraft)").arg(rowCount()).arg(unfilteredCount);
    }

    return tr("(%1 aircraft)").arg(unfilteredCount);
}

void AircraftProxyModel::setInstalledFilterEnabled(bool e)
{
    if (e == m_onlyShowInstalled) {
        return;
    }

    m_onlyShowInstalled = e;
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

