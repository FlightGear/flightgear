#ifndef QMLAIRCRAFTINFO_HXX
#define QMLAIRCRAFTINFO_HXX

#include <memory>

#include <QObject>
#include <QUrl>
#include <QSharedPointer>
#include <QAbstractListModel>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>

struct AircraftItem;
typedef QSharedPointer<AircraftItem> AircraftItemPtr;

class QmlAircraftInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(int variant READ variant WRITE setVariant NOTIFY variantChanged)

    Q_PROPERTY(QVariantList previews READ previews NOTIFY infoChanged)
    Q_PROPERTY(int numVariants READ numVariants NOTIFY infoChanged)

    Q_PROPERTY(QString name READ name NOTIFY infoChanged)
    Q_PROPERTY(QString description READ description NOTIFY infoChanged)
    Q_PROPERTY(QString authors READ authors NOTIFY infoChanged)

    Q_PROPERTY(QUrl thumbnail READ thumbnail NOTIFY infoChanged)

    Q_PROPERTY(QString pathOnDisk READ pathOnDisk NOTIFY infoChanged)

    Q_PROPERTY(QString packageId READ packageId NOTIFY infoChanged)

    Q_PROPERTY(int packageSize READ packageSize NOTIFY infoChanged)

    Q_PROPERTY(int downloadedBytes READ downloadedBytes NOTIFY downloadChanged)

    Q_PROPERTY(QVariant status READ status NOTIFY infoChanged)
    Q_PROPERTY(QVariant installStatus READ installStatus NOTIFY downloadChanged)

    Q_PROPERTY(QString minimumFGVersion READ minimumFGVersion NOTIFY infoChanged)

    Q_INVOKABLE void requestInstallUpdate();

    Q_INVOKABLE void requestUninstall();

    Q_INVOKABLE void requestInstallCancel();

    Q_PROPERTY(QVariantList ratings READ ratings NOTIFY infoChanged)

    Q_PROPERTY(QStringList variantNames READ variantNames NOTIFY infoChanged)

    Q_PROPERTY(bool isPackaged READ isPackaged NOTIFY infoChanged)

    Q_PROPERTY(bool hasStates READ hasStates NOTIFY infoChanged)

    Q_PROPERTY(QAbstractListModel* statesModel READ statesModel NOTIFY infoChanged)

public:
    explicit QmlAircraftInfo(QObject *parent = nullptr);
    virtual ~QmlAircraftInfo();

    QUrl uri() const;

    int numVariants() const;

    QString name() const;
    QString description() const;
    QString authors() const;
    QVariantList ratings() const;

    QVariantList previews() const;

    QUrl thumbnail() const;
    QString pathOnDisk() const;

    QString packageId() const;
    int packageSize() const;
    int downloadedBytes() const;

    QVariant status() const;
    QString minimumFGVersion() const;

    static QVariant packageAircraftStatus(simgear::pkg::PackageRef p);

    int variant() const
    {
        return _variant;
    }

    QVariant installStatus() const;

     simgear::pkg::PackageRef packageRef() const;

    void setDownloadBytes(int bytes);

    QStringList variantNames() const;

    bool isPackaged() const;

    bool hasStates() const
    {
        return !_statesModel.isNull();
    }

    static const int StateTagRole;
    static const int StateDescriptionRole;
    static const int StateExplicitRole;

    QAbstractListModel* statesModel();
signals:
    void uriChanged();
    void infoChanged();
    void downloadChanged();
    void variantChanged(int variant);

public slots:

    void setUri(QUrl uri);

    void setVariant(int variant);

private:
    AircraftItemPtr resolveItem() const;
    void checkForStates();

    class Delegate;
    std::unique_ptr<Delegate> _delegate;

    simgear::pkg::PackageRef _package;
    AircraftItemPtr _item;
    int _variant = 0;
    int _downloadBytes = 0;
    QScopedPointer<QAbstractListModel> _statesModel;
};

#endif // QMLAIRCRAFTINFO_HXX
