#ifndef AIRCRAFTPREVIEWWINDOW_H
#define AIRCRAFTPREVIEWWINDOW_H

#include <QWidget>
#include <QUrl>

class AircraftPreviewWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AircraftPreviewWindow(QWidget *parent = 0);

    void setUrls(QList<QUrl> urls);
signals:

public slots:

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private:
    unsigned int m_currentIndex = 0;

    QList<QUrl> m_urls;
    QList<QPixmap> m_pixmaps; // lazily populated
};

#endif // AIRCRAFTPREVIEWWINDOW_H
