#ifndef THUMBNAILIMAGEITEM_HXX
#define THUMBNAILIMAGEITEM_HXX

#include <memory>

#include <QQuickItem>
#include <QUrl>
#include <QImage>

class ThumbnailImageItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QString aircraftUri READ aircraftUri WRITE setAircraftUri NOTIFY aircraftUriChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY aircraftUriChanged)

    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

    Q_PROPERTY(QSize maximumSize READ maximumSize WRITE setMaximumSize NOTIFY maximumSizeChanged)
public:
    ThumbnailImageItem(QQuickItem* parent = nullptr);
    ~ThumbnailImageItem();

    QSGNode* updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    QUrl url() const;

    QString aircraftUri() const;

    QSize sourceSize() const;

    QSize maximumSize() const;

signals:
    void aircraftUriChanged();

    void sourceSizeChanged();

    void maximumSizeChanged(QSize maximumSize);

public slots:

    void setAircraftUri(QString uri);

    void setMaximumSize(QSize maximumSize);

private:
    class ThumbnailPackageDelegate;
    friend class ThumbnailPackageDelegate;

    void setImage(QImage image);
    void clearImage();

    QString m_aircraftUri;
    QUrl m_imageUrl;
    std::unique_ptr<ThumbnailPackageDelegate> m_delegate;

    bool m_imageDirty = false;
    QImage m_image;
    QSize m_maximumSize;
};

#endif // THUMBNAILIMAGEITEM_HXX
