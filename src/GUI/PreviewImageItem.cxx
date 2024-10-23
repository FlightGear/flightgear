#include "PreviewImageItem.hxx"

#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QFileInfo>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <Main/globals.hxx>

namespace {

QNetworkAccessManager* global_previewNetAccess = nullptr;

}

PreviewImageItem::PreviewImageItem(QQuickItem* parent) :
    QQuickItem(parent)
{
    setFlag(ItemHasContents);
  //  setImplicitWidth(STANDARD_THUMBNAIL_WIDTH);
   // setImplicitHeight(STANDARD_THUMBNAIL_HEIGHT);

    Q_ASSERT(global_previewNetAccess);
}

PreviewImageItem::~PreviewImageItem()
{
}

QSGNode *PreviewImageItem::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData *)
{
    if (m_image.isNull()) {
        delete oldNode;
        return nullptr;
    }

    QSGSimpleTextureNode* textureNode = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (m_imageDirty || !textureNode) {
        if (!textureNode) {
            textureNode = new QSGSimpleTextureNode;
            textureNode->setOwnsTexture(true);
        }

        QSGTexture* tex = window()->createTextureFromImage(m_image);
        textureNode->setTexture(tex);
        textureNode->markDirty(QSGBasicGeometryNode::DirtyMaterial);
        m_imageDirty = false;
    }

    textureNode->setRect(QRectF(0, 0, width(), height()));
    return textureNode;
}

QUrl PreviewImageItem::imageUrl() const
{
    return m_imageUrl;
}

QSize PreviewImageItem::sourceSize() const
{
    return m_image.size();
}

void PreviewImageItem::setGlobalNetworkAccess(QNetworkAccessManager *netAccess)
{
    global_previewNetAccess = netAccess;
}

bool PreviewImageItem::isLoading() const
{
    return m_requestActive;
}

float PreviewImageItem::aspectRatio() const
{
    return static_cast<float>(m_image.width()) / m_image.height();
}

void PreviewImageItem::clear()
{
    m_imageUrl.clear();
    m_image = QImage{};
    m_imageDirty = true;
    m_requestActive = false;
    update();
    emit imageUrlChanged();
    emit isLoadingChanged();
}

void PreviewImageItem::setImageUrl( QUrl url)
{
    if (m_imageUrl == url)
        return;

    m_imageUrl = url;
    m_downloadRetryCount = 0;
    startDownload();
    emit imageUrlChanged();
}

void PreviewImageItem::startDownload()
{
    if (m_imageUrl.isEmpty())
        return;

    QNetworkRequest request(m_imageUrl);
    QNetworkReply* reply = global_previewNetAccess->get(request);
    connect(reply, &QNetworkReply::finished, this, &PreviewImageItem::onFinished);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(reply, &QNetworkReply::errorOccurred,
            this, &PreviewImageItem::onDownloadError);
#else
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onDownloadError(QNetworkReply::NetworkError)));
#endif
    m_requestActive = true;
    emit isLoadingChanged();
}

void PreviewImageItem::setImage(QImage image)
{
    m_image = image;
    m_imageDirty = true;
    setImplicitSize(m_image.width(), m_image.height());
    emit sourceSizeChanged();
    update();
}

void PreviewImageItem::onFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->url() != m_imageUrl) {
        // if replies arrive out of order, don't trample the correct one
        return;
    }

    QImage img;
    if (!img.load(reply, nullptr)) {
        qWarning() << Q_FUNC_INFO << "failed to read image data from" << reply->url();
        return;
    }
    setImage(img);
    m_requestActive = false;
    emit isLoadingChanged();
}

void PreviewImageItem::onDownloadError(QNetworkReply::NetworkError errorCode)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (errorCode == 403) {
        if (m_downloadRetryCount++ < 4) {
            startDownload(); // retry
            return;
        }
    }

    qWarning() << Q_FUNC_INFO << "failed to download:" << reply->url();
    qWarning() << "\t" << reply->errorString();
    m_requestActive = false;
    emit isLoadingChanged();
}
