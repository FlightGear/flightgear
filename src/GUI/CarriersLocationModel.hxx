#ifndef CARRIERSMODEL_H
#define CARRIERSMODEL_H

#include <vector>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/props/props.hxx>

#include <QAbstractListModel>
#include <QPixmap>

class CarriersLocationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CarriersLocationModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    // copied from NavaidSearchModel
    enum Roles {
        GeodRole = Qt::UserRole + 1,
        GuidRole = Qt::UserRole + 2,
        IdentRole = Qt::UserRole + 3,
        NameRole = Qt::UserRole + 4,
        IconRole = Qt::UserRole + 5,
        TypeRole = Qt::UserRole + 6,
        NavFrequencyRole = Qt::UserRole + 7,
        DescriptionRole = Qt::UserRole + 8
    };

    int indexOf(const QString name) const;

    SGGeod geodForIndex(int index) const;
    QString pennantForIndex(int index) const;

    QStringList parkingsForIndex(int index) const;
private:
    mutable QPixmap m_carrierPixmap;

    struct Carrier
    {
        QString mScenario; // scenario ID for loading
        QString mCallsign; // pennant-number
        QString mName;
        QString mDescription;
        SGGeod mInitialLocation;
        // icon?
        QString mTACAN;
        QStringList mParkings;
    };

    using CarrierVec = std::vector<Carrier>;
    CarrierVec mCarriers;

    void processCarrier(const std::string& scenario, SGPropertyNode_ptr carrierNode);
};

#endif // CARRIERSMODEL_H
