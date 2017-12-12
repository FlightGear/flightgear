#ifndef AIRCRAFTSEARCHFILTERMODEL_HXX
#define AIRCRAFTSEARCHFILTERMODEL_HXX

#include <QSortFilterProxyModel>

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

    /**
      * Compute the row (index in QML / ListView speak) based on an aircraft URI.
      * Return -1 if the UIR is not present in the (filtered) model
      **/
    Q_INVOKABLE int indexForURI(QUrl uri) const;

    Q_INVOKABLE void selectVariantForAircraftURI(QUrl uri);

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
signals:
    void ratingsChanged();
    void ratingsFilterEnabledChanged();
    void summaryTextChanged();

public slots:

    void setInstalledFilterEnabled(bool e);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool filterAircraft(const QModelIndex& sourceIndex) const;

    bool m_ratingsFilter = true;
    bool m_onlyShowInstalled = false;
    QList<int> m_ratings;
    QString m_filterString;
    SGPropertyNode_ptr m_filterProps;
};

#endif // AIRCRAFTSEARCHFILTERMODEL_HXX
