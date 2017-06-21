#include "PreviewWindow.hxx"

#include <QPainter>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QDebug>

const int BORDER_SIZE = 16;

PreviewWindow::PreviewWindow(QWidget *parent)
    : QDialog(parent)
    , m_netAccess(new QNetworkAccessManager(this))
{
    setWindowFlags(Qt::Popup);
    setModal(true);

    m_closeIcon.load(":/preview/close-icon");
    m_leftIcon.load(":/preview/left-arrow-icon");
    m_rightIcon.load(":/preview/right-arrow-icon");
}

void PreviewWindow::setUrls(QVariantList urls)
{
    m_cache.clear();

    Q_FOREACH (QVariant v, urls) {
        QUrl url = v.toUrl();
        QNetworkReply* reply = m_netAccess->get(QNetworkRequest(url));
        qInfo() << "requesting:" << url;
        connect(reply, &QNetworkReply::finished, this, &PreviewWindow::onDownloadFinished);
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onDownloadError(QNetworkReply::NetworkError)));
    }
}

void PreviewWindow::paintEvent(QPaintEvent *pe)
{
    QUrl key = m_urls.at(m_currentPreview);
    QPixmap pm = m_cache.value(key.toString());

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    QRect imgRect = rect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);
    painter.drawPixmap(imgRect, pm);

    QRect closeIconRect = m_closeIcon.rect();
    closeIconRect.moveTopRight(rect().topRight());
    painter.drawPixmap(closeIconRect, m_closeIcon);

    QRect leftArrowRect = m_leftIcon.rect();
    unsigned int iconTop = rect().center().y() - (m_leftIcon.size().height() / 2);
    leftArrowRect.moveTopLeft(QPoint(0, iconTop));
    painter.drawPixmap(leftArrowRect, m_leftIcon);

    QRect rightArrowRect = m_rightIcon.rect();
    rightArrowRect.moveTopRight(QPoint(width(), iconTop));
    painter.drawPixmap(rightArrowRect, m_rightIcon);
}

void PreviewWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QRect closeIconRect = m_closeIcon.rect();
    closeIconRect.moveTopRight(rect().topRight());

    QRect leftArrowRect = m_leftIcon.rect();
    unsigned int iconTop = rect().center().y() - (m_leftIcon.size().height() / 2);
    leftArrowRect.moveTopLeft(QPoint(0, iconTop));

    QRect rightArrowRect = m_rightIcon.rect();
    rightArrowRect.moveTopRight(QPoint(width(), iconTop));

    if (closeIconRect.contains(event->pos())) {
        close();
        deleteLater();
    }

    if (leftArrowRect.contains(event->pos())) {
        m_currentPreview = (m_currentPreview - 1) % m_urls.size();
        update();
    }

    if (rightArrowRect.contains(event->pos())) {
        m_currentPreview = (m_currentPreview + 1) % m_urls.size();
        update();
    }
}

void PreviewWindow::onDownloadFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    QImage img;
    if (!img.load(reply, nullptr)) {
        qWarning() << Q_FUNC_INFO << "failed to read image data from" << reply->url();
        return;
    }

    m_cache.insert(reply->url().toString(), QPixmap::fromImage(img));
    m_urls.append(reply->url());

    if (!isVisible()) {
        QSize winSize(img.width() + BORDER_SIZE * 2, img.height() + BORDER_SIZE * 2);
        resize(winSize);

        show();
    }
}

void PreviewWindow::onDownloadError(QNetworkReply::NetworkError errorCode)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    qWarning() << "failed to download:" << reply->url();
    qWarning() << reply->errorString();
    m_urls.removeOne(reply->url());
}
