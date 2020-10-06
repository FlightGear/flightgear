#ifndef QMLAIRCRAFTINFO_HXX
#define QMLAIRCRAFTINFO_HXX

#include <memory>

#include <QObject>
#include <QUrl>
#include <QSharedPointer>
#include <QAbstractListModel>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>

#include "UnitsModel.hxx"

struct AircraftItem;
typedef QSharedPointer<AircraftItem> AircraftItemPtr;

class StatesModel;

class QmlAircraftInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(quint32 variant READ variant WRITE setVariant NOTIFY variantChanged)

    Q_PROPERTY(QVariantList previews READ previews NOTIFY infoChanged)
    Q_PROPERTY(quint32 numVariants READ numVariants NOTIFY infoChanged)
    Q_PROPERTY(QStringList variantNames READ variantNames NOTIFY infoChanged)

    Q_PROPERTY(QString name READ name NOTIFY infoChanged)
    Q_PROPERTY(QString description READ description NOTIFY infoChanged)
    Q_PROPERTY(QString authors READ authors NOTIFY infoChanged)
    Q_PROPERTY(QUrl thumbnail READ thumbnail NOTIFY infoChanged)
    Q_PROPERTY(QVariantList ratings READ ratings NOTIFY infoChanged)

    Q_PROPERTY(QString pathOnDisk READ pathOnDisk NOTIFY infoChanged)
    Q_PROPERTY(QString packageId READ packageId NOTIFY infoChanged)
    Q_PROPERTY(quint64 packageSize READ packageSize NOTIFY infoChanged)
    Q_PROPERTY(quint64 downloadedBytes READ downloadedBytes NOTIFY downloadChanged)
    Q_PROPERTY(bool isPackaged READ isPackaged NOTIFY infoChanged)

    Q_PROPERTY(QVariant status READ status NOTIFY infoChanged)
    Q_PROPERTY(QVariant installStatus READ installStatus NOTIFY downloadChanged)

    Q_PROPERTY(QString minimumFGVersion READ minimumFGVersion NOTIFY infoChanged)

    Q_PROPERTY(QUrl homePage READ homePage NOTIFY infoChanged)
    Q_PROPERTY(QUrl supportUrl READ supportUrl NOTIFY infoChanged)
    Q_PROPERTY(QUrl wikipediaUrl READ wikipediaUrl NOTIFY infoChanged)

    Q_PROPERTY(QuantityValue cruiseSpeed READ cruiseSpeed NOTIFY infoChanged)
    Q_PROPERTY(QuantityValue cruiseAltitude READ cruiseAltitude NOTIFY infoChanged)
    Q_PROPERTY(QuantityValue approachSpeed READ approachSpeed NOTIFY infoChanged)

    Q_PROPERTY(QString icaoType READ icaoType NOTIFY infoChanged)

    Q_PROPERTY(bool hasStates READ hasStates NOTIFY infoChanged)
    Q_PROPERTY(StatesModel* statesModel READ statesModel CONSTANT)

    Q_PROPERTY(bool favourite READ favourite WRITE setFavourite NOTIFY favouriteChanged)
public:
    explicit QmlAircraftInfo(QObject *parent = nullptr);
    virtual ~QmlAircraftInfo();

    QUrl uri() const;

    quint32 numVariants() const;

    QString name() const;
    QString description() const;
    QString authors() const;
    QVariantList ratings() const;

    QVariantList previews() const;

    QUrl thumbnail() const;
    QString pathOnDisk() const;

    QUrl homePage() const;
    QUrl supportUrl() const;
    QUrl wikipediaUrl() const;

    QString packageId() const;
    quint64 packageSize() const;
    quint64 downloadedBytes() const;

    QVariant status() const;
    QString minimumFGVersion() const;

    static QVariant packageAircraftStatus(simgear::pkg::PackageRef p);

    quint32 variant() const
    {
        return _variant;
    }

    QVariant installStatus() const;

     simgear::pkg::PackageRef packageRef() const;

    void setDownloadBytes(quint64 bytes);

    QStringList variantNames() const;

    bool isPackaged() const;

    bool hasStates() const;

    bool hasState(QString name) const;

    bool haveExplicitAutoState() const;

    static const int StateTagRole;
    static const int StateDescriptionRole;
    static const int StateExplicitRole;

    StatesModel* statesModel();

    QuantityValue cruiseSpeed() const;
    QuantityValue approachSpeed() const;
    QuantityValue cruiseAltitude() const;

    QString icaoType() const;

    Q_INVOKABLE bool isSpeedBelowLimits(QuantityValue speed) const;
    Q_INVOKABLE bool isAltitudeBelowLimits(QuantityValue speed) const;

    Q_INVOKABLE bool hasTag(QString tag) const;
    bool favourite() const;

signals:
    void uriChanged();
    void infoChanged();
    void downloadChanged();
    void variantChanged(quint32 variant);
    void favouriteChanged();

public slots:

    void setUri(QUrl uri);

    void setVariant(quint32 variant);

    void setFavourite(bool favourite);

private slots:
    void onFavouriteChanged(QUrl u);
    void retryValidateLocalProps();

private:
    AircraftItemPtr resolveItem() const;

    void validateStates() const;
    void validateLocalProps() const;

    class Delegate;
    std::unique_ptr<Delegate> _delegate;

    simgear::pkg::PackageRef _package;
    AircraftItemPtr _item;
    quint32 _variant = 0;
    quint64 _downloadBytes = 0;

    /// either null or owned by us
    mutable StatesModel* _statesModel = nullptr;

    mutable bool _statesChecked = false;

    /// if the aircraft is locally installed, this is the cached
    /// parsed contents of the -set.xml.
    mutable SGPropertyNode_ptr _cachedProps;
};

#endif // QMLAIRCRAFTINFO_HXX
