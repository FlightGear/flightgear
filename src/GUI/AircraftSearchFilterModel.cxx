#include "AircraftSearchFilterModel.hxx"

#include "AircraftModel.hxx"
#include <simgear/package/Package.hxx>

AircraftProxyModel::AircraftProxyModel(QObject *pr) :
    QSortFilterProxyModel(pr)
{
}

void AircraftProxyModel::setRatings(int *ratings)
{
    ::memcpy(m_ratings, ratings, sizeof(int) * 4);
    invalidate();
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

void AircraftProxyModel::setRatingFilterEnabled(bool e)
{
    if (e == m_ratingsFilter) {
        return;
    }

    m_ratingsFilter = e;
    invalidate();
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
    AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
    if (status == MessageWidget) {
        return true;
    }

    if (!filterAircraft(index)) {
        return false;
    }

    if (m_onlyShowInstalled) {
        QVariant v = index.data(AircraftPackageStatusRole);
        AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
        if (status == PackageNotInstalled) {
            return false;
        }
    }

    if (!m_onlyShowInstalled && m_ratingsFilter) {
        for (int i=0; i<4; ++i) {
            if (m_ratings[i] > index.data(AircraftRatingRole + i).toInt()) {
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

