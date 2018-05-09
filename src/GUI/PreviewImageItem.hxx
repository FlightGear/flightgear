#ifndef PREVIEW_IMAGEITEM_HXX
#define PREVIEW_IMAGEITEM_HXX

#include <memory>

#include <QQuickItem>
#include <QUrl>
#include <QImage>
#include <QNetworkReply>

class QNetworkAccessManager;

class PreviewImageItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QUrl imageUrl READ imageUrl WRITE setImageUrl NOTIFY imageUrlChanged)

    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

    Q_PROPERTY(float aspectRatio READ aspectRatio NOTIFY sourceSizeChanged)
public:
    PreviewImageItem(QQuickItem* parent = nullptr);
    ~PreviewImageItem();

    QSGNode* updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    QUrl imageUrl() const;

    QSize sourceSize() const;

    static void setGlobalNetworkAccess(QNetworkAccessManager* netAccess);

    bool isLoading() const;

    float aspectRatio() const;

    /**
      @brief clear the image immediately, so we don't see a stale / expired
      one while attemtping to load the next one
      */
    Q_INVOKABLE void clear();
signals:
    void imageUrlChanged();
    void sourceSizeChanged();
    void isLoadingChanged();

public slots:

    void setImageUrl(QUrl url);

private slots:
    void onDownloadError(QNetworkReply::NetworkError errorCode);

    void onFinished();
private:
    void setImage(QImage image);
    void startDownload();

    QUrl m_imageUrl;

    bool m_imageDirty = false;
    QImage m_image;
    unsigned int m_downloadRetryCount = 0;
    bool m_requestActive = false;
};

#endif // PREVIEW_IMAGEITEM_HXX
