#ifndef FGQCANVASIMAGELOADER_H
#define FGQCANVASIMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QNetworkReply>

class QNetworkAccessManager;

class FGQCanvasImageLoader : public QObject
{
    Q_OBJECT
public:
    static FGQCanvasImageLoader* instance();

    static void initialise(QNetworkAccessManager* dl);

    void setHost(QString hostName, int portNumber);

    QPixmap getImage(const QByteArray& imagePath);

    /**
     * @brief connectToImageLoaded - allow images to discover when they are loaded
     * @param imagePath - FGFS host relative path as found in the canvas (will be resolved
     * against aircraft-data, and potentially other places)
     * @param receiver - normal connect() receiver object
     * @param slot - slot macro
     */
    void connectToImageLoaded(const QByteArray& imagePath, QObject* receiver, const char* slot);
signals:


private:
    explicit FGQCanvasImageLoader(QNetworkAccessManager* dl);

    void onDownloadFinished();

private:
    QNetworkAccessManager* m_downloader;
    QString m_hostName;
    int m_port;

    QMap<QByteArray, QPixmap> m_cache;
    QList<QNetworkReply*> m_transfers;

};

#endif // FGQCANVASIMAGELOADER_H
