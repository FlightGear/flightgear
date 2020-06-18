#include "ThumbnailImageItem.hxx"

#include <QSGSimpleTextureNode>
#include <QQuickWindow>
#include <QFileInfo>
#include <QDir>

#include <Main/globals.hxx>

#include <simgear/package/Root.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>

using namespace simgear;

const int STANDARD_THUMBNAIL_HEIGHT = 128;
const int STANDARD_THUMBNAIL_WIDTH = 172;

class ThumbnailImageItem::ThumbnailPackageDelegate : public pkg::Delegate
{
public:
    ThumbnailPackageDelegate(ThumbnailImageItem* o) : owner(o) {}

    void catalogRefreshed(pkg::CatalogRef, StatusCode) override {}
    void startInstall(pkg::InstallRef) override {}
    void installProgress(pkg::InstallRef, unsigned int, unsigned int) override {}
    void finishInstall(pkg::InstallRef, StatusCode ) override {}
    void dataForThumbnail(const std::string& aThumbnailUrl,
                          size_t length, const uint8_t* bytes) override;

    ThumbnailImageItem* owner;
};

void ThumbnailImageItem::ThumbnailPackageDelegate::dataForThumbnail(const std::string& aThumbnailUrl,
        size_t length, const uint8_t* bytes)
{
    if (aThumbnailUrl != owner->url().toString().toStdString()) {
        return;
    }

    const auto iLength = static_cast<int>(length);
    QImage img = QImage::fromData(QByteArray::fromRawData(reinterpret_cast<const char*>(bytes), iLength));
    if (img.isNull()) {
        if (length > 0) {
            // warn if we had valid bytes but couldn't load it, i.e corrupted data or similar
            qWarning() << "failed to load image data for URL:" << QString::fromStdString(aThumbnailUrl);
            owner->clearImage();
        }
        return;
    }

    owner->setImage(img);
}

ThumbnailImageItem::ThumbnailImageItem(QQuickItem* parent) :
    QQuickItem(parent),
    m_delegate(new ThumbnailPackageDelegate(this)),
    m_maximumSize(9999, 9999)
{
    globals->packageRoot()->addDelegate(m_delegate.get());
    setFlag(ItemHasContents);
    setImplicitWidth(STANDARD_THUMBNAIL_WIDTH);
    setImplicitHeight(STANDARD_THUMBNAIL_HEIGHT);
}

ThumbnailImageItem::~ThumbnailImageItem()
{
    globals->packageRoot()->removeDelegate(m_delegate.get());
}

QSGNode *ThumbnailImageItem::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData *)
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
        QSGTexture* tex = window()->createTextureFromImage(m_image, QQuickWindow::TextureIsOpaque);
        textureNode->setTexture(tex);
        textureNode->markDirty(QSGBasicGeometryNode::DirtyMaterial);
        m_imageDirty = false;
    }

    textureNode->setRect(QRectF(0, 0, width(), height()));
    return textureNode;
}

QUrl ThumbnailImageItem::url() const
{
    return m_imageUrl;
}

QString ThumbnailImageItem::aircraftUri() const
{
    return m_aircraftUri;
}

QSize ThumbnailImageItem::sourceSize() const
{
    return m_image.size();
}

QSize ThumbnailImageItem::maximumSize() const
{
    return m_maximumSize;
}

void ThumbnailImageItem::setAircraftUri(QString uri)
{
    if (m_aircraftUri == uri)
        return;

    m_aircraftUri = uri;

    if (uri.startsWith("package:")) {
        const std::string packageId = m_aircraftUri.toStdString().substr(8);
        pkg::Root* root = globals->packageRoot();
        pkg::PackageRef package = root->getPackageById(packageId);
        if (package) {
            auto variant = package->indexOfVariant(packageId);
            const auto thumbnail = package->thumbnailForVariant(variant);
            m_imageUrl = QUrl(QString::fromStdString(thumbnail.url));
            if (m_imageUrl.isValid()) {
                globals->packageRoot()->requestThumbnailData(m_imageUrl.toString().toStdString());
            } else {
                clearImage();
            }
        }
    } else {
        QFileInfo aircraftSetPath(QUrl(uri).toLocalFile());
        const QString thumbnailPath = aircraftSetPath.dir().filePath("thumbnail.jpg");
        m_imageUrl = QUrl::fromLocalFile(thumbnailPath);

        if (QFileInfo(thumbnailPath).exists()) {
            QImage img;
            if (img.load(thumbnailPath)) {
                setImage(img);
            } else {
                qWarning() << Q_FUNC_INFO << "failed to load thumbnail from:" << thumbnailPath;
                clearImage();
            }
        }
    } // of local aircraft case

    emit aircraftUriChanged();
}

void ThumbnailImageItem::setMaximumSize(QSize maximumSize)
{
    if (m_maximumSize == maximumSize)
        return;

    m_maximumSize = maximumSize;
    emit maximumSizeChanged(m_maximumSize);

    if (!m_image.isNull()) {
        setImplicitSize(qMin(m_maximumSize.width(), m_image.width()),
                        qMin(m_maximumSize.height(), m_image.height()));
    }
}

void ThumbnailImageItem::setImage(QImage image)
{
    m_image = image;
    m_imageDirty = true;
    setImplicitSize(qMin(m_maximumSize.width(), m_image.width()),
                    qMin(m_maximumSize.height(), m_image.height()));
    emit sourceSizeChanged();
    update();
}

void ThumbnailImageItem::clearImage()
{
    m_image = QImage{};
    m_imageDirty = true;
    setImplicitSize(m_maximumSize.width(), m_maximumSize.height());
    emit sourceSizeChanged();
    update();
}
