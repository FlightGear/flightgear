#include "fgqcanvasimageloader.h"

#include <QDebug>
#include <QNetworkAccessManager>

static FGQCanvasImageLoader* static_instance = nullptr;

class TransferSignalHolder : public QObject
{
    Q_OBJECT
public:
    TransferSignalHolder(QObject* pr) : QObject(pr) { }
signals:
    void trigger();
};

FGQCanvasImageLoader::FGQCanvasImageLoader(QNetworkAccessManager* dl)
    : m_downloader(dl)
{
}

void FGQCanvasImageLoader::onDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    QPixmap pm;
    if (!pm.loadFromData(reply->readAll())) {
        qWarning() << "image loading failed";
    } else {
        qDebug() << "did download:" << reply->property("image").toByteArray();
        m_cache.insert(reply->property("image").toByteArray(), pm);

        TransferSignalHolder* signalHolder = reply->findChild<TransferSignalHolder*>("holder");
        if (signalHolder) {
            qDebug() << "triggering image updates";
            signalHolder->trigger();
        }
    }

    m_transfers.removeOne(reply);
    reply->deleteLater();
}

FGQCanvasImageLoader *FGQCanvasImageLoader::instance()
{
    return static_instance;
}

void FGQCanvasImageLoader::initialise(QNetworkAccessManager *dl)
{
    Q_ASSERT(static_instance == nullptr);
    static_instance = new FGQCanvasImageLoader(dl);
}

void FGQCanvasImageLoader::setHost(QString hostName, int portNumber)
{
    m_hostName = hostName;
    m_port = portNumber;
}

QPixmap FGQCanvasImageLoader::getImage(const QByteArray &imagePath)
{
    if (m_cache.contains(imagePath)) {
        // cached, easy
        return m_cache.value(imagePath);
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostName);
    url.setPort(m_port);
    url.setPath("/aircraft-dir/" + imagePath);

    Q_FOREACH (QNetworkReply* transfer, m_transfers) {
        if (transfer->url() == url) {
            return QPixmap(); // transfer already active
        }
    }

    qDebug() << "requesting image" << url;
    QNetworkReply* reply = m_downloader->get(QNetworkRequest(url));
    reply->setProperty("image", imagePath);

    connect(reply, &QNetworkReply::finished, this, &FGQCanvasImageLoader::onDownloadFinished);
    m_transfers.append(reply);

    return QPixmap();
}

void FGQCanvasImageLoader::connectToImageLoaded(const QByteArray &imagePath, QObject *receiver, const char *slot)
{
    Q_FOREACH (QNetworkReply* transfer, m_transfers) {
        if (transfer->property("image").toByteArray() == imagePath) {
            QObject* signalHolder = transfer->findChild<QObject*>("holder");
            if (!signalHolder) {
                signalHolder = new TransferSignalHolder(transfer);
                signalHolder->setObjectName("holder");
            }

            connect(signalHolder, SIGNAL(trigger()), receiver, slot);
            return;
        }
    }

    qWarning() << "no transfer active for" << imagePath;
}

#include "fgqcanvasimageloader.moc"
