#ifndef QMLAIRCRAFTINFO_HXX
#define QMLAIRCRAFTINFO_HXX

#include <QObject>
#include <QUrl>
#include <QSharedPointer>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>

struct AircraftItem;
typedef QSharedPointer<AircraftItem> AircraftItemPtr;

class QmlAircraftInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl uri READ uri WRITE setUri NOTIFY uriChanged)

    Q_PROPERTY(int numPreviews READ numPreviews NOTIFY infoChanged)
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

    Q_PROPERTY(QString minimumFGVersion READ minimumFGVersion NOTIFY infoChanged)

    Q_INVOKABLE void requestInstallUpdate();

    Q_INVOKABLE void requestUninstall();

    Q_INVOKABLE void requestInstallCancel();

    Q_PROPERTY(QVariantList ratings READ ratings NOTIFY infoChanged)

public:
    explicit QmlAircraftInfo(QObject *parent = nullptr);
    virtual ~QmlAircraftInfo();

    QUrl uri() const
    {
        return _uri;
    }

    int numPreviews() const;
    int numVariants() const;

    QString name() const;
    QString description() const;
    QString authors() const;
    QVariantList ratings() const;

    QUrl thumbnail() const;
    QString pathOnDisk() const;

    QString packageId() const;
    int packageSize() const;
    int downloadedBytes() const;

    QVariant status() const;
    QString minimumFGVersion() const;

    static QVariant packageAircraftStatus(simgear::pkg::PackageRef p);
signals:
    void uriChanged();
    void infoChanged();
    void downloadChanged();
public slots:

    void setUri(QUrl uri);

private:
    QUrl _uri;
    simgear::pkg::PackageRef _package;
    AircraftItemPtr _item;
    int _variant = 0;

    AircraftItemPtr resolveItem() const;
};

#endif // QMLAIRCRAFTINFO_HXX
