#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QDialog>

#include <QVariant>
#include <QNetworkAccessManager>
#include <QMap>

class PreviewWindow : public QDialog
{
    Q_OBJECT
public:
    explicit PreviewWindow(QWidget *parent = 0);

    void setUrls(QVariantList urls);

signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent *pe) override;

    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void onDownloadFinished();


    unsigned int m_currentPreview = 0;
    QNetworkAccessManager* m_netAccess;
    QList<QUrl> m_urls;
    QMap<QUrl, QPixmap> m_cache;

    QPixmap m_leftIcon, m_rightIcon, m_closeIcon;
};

#endif // PREVIEWWINDOW_H
