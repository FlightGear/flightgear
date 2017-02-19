#ifndef AIRCRAFTSEARCHFILTERMODEL_HXX
#define AIRCRAFTSEARCHFILTERMODEL_HXX

#include <QSortFilterProxyModel>

#include <simgear/props/props.hxx>

class AircraftProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AircraftProxyModel(QObject* pr);

    void setRatings(int* ratings);

    void setAircraftFilterString(QString s);

public slots:
    void setRatingFilterEnabled(bool e);

    void setInstalledFilterEnabled(bool e);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool filterAircraft(const QModelIndex& sourceIndex) const;

    bool m_ratingsFilter = true;
    bool m_onlyShowInstalled = false;
    int m_ratings[4] = {3, 3, 3, 3};
    QString m_filterString;
    SGPropertyNode_ptr m_filterProps;
};

#endif // AIRCRAFTSEARCHFILTERMODEL_HXX
