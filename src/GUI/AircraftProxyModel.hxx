#ifndef AIRCRAFTSEARCHFILTERMODEL_HXX
#define AIRCRAFTSEARCHFILTERMODEL_HXX

#include <QSortFilterProxyModel>
#include <QUrl>

#include <simgear/props/props.hxx>

class AircraftProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AircraftProxyModel(QObject* pr, QAbstractItemModel * source);

    Q_PROPERTY(QList<int> ratings READ ratings WRITE setRatings NOTIFY ratingsChanged)

    Q_PROPERTY(bool ratingsFilterEnabled READ ratingsFilterEnabled WRITE setRatingFilterEnabled NOTIFY ratingsFilterEnabledChanged)

    Q_PROPERTY(QString summaryText READ summaryText NOTIFY summaryTextChanged)

    Q_INVOKABLE void setAircraftFilterString(QString s);

    Q_PROPERTY(int count READ count NOTIFY countChanged)

    /**
      * Compute the row (index in QML / ListView speak) based on an aircraft URI.
      * Return -1 if the UIR is not present in the (filtered) model
      **/
    Q_INVOKABLE int indexForURI(QUrl uri) const;

    Q_INVOKABLE void selectVariantForAircraftURI(QUrl uri);

    Q_INVOKABLE void loadRatingsSettings();

    Q_INVOKABLE void saveRatingsSettings();

    QList<int> ratings() const
    {
        return m_ratings;
    }

    bool ratingsFilterEnabled() const
    {
        return m_ratingsFilter;
    }

    void setRatings(QList<int> ratings);
    void setRatingFilterEnabled(bool e);

    QString summaryText() const;

    int count() const;
signals:
    void ratingsChanged();
    void ratingsFilterEnabledChanged();
    void summaryTextChanged();
    void countChanged();

public slots:

    void setInstalledFilterEnabled(bool e);

    void setHaveUpdateFilterEnabled(bool e);

    void setShowFavourites(bool e);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    bool filterAircraft(const QModelIndex& sourceIndex) const;

    bool m_ratingsFilter = false;
    bool m_onlyShowInstalled = false;
    bool m_onlyShowWithUpdate = false;
    bool m_onlyShowFavourites = false;

    QList<int> m_ratings;
    QString m_filterString;
    SGPropertyNode_ptr m_filterProps;
};

#endif // AIRCRAFTSEARCHFILTERMODEL_HXX
