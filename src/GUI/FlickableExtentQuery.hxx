#ifndef FLICKABLEEXTENTQUERY_HXX
#define FLICKABLEEXTENTQUERY_HXX

#include <QObject>
#include <QQuickItem>

/**
 * @brief FlickableExtentQuery exists to expose some unfortunately private
 * information from a Flickable, to mimic what QQC2 Scrollbar does internlly.
 */
class FlickableExtentQuery : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem* flickable READ flickable WRITE setFlickable NOTIFY flickableChanged)

public:
    explicit FlickableExtentQuery(QObject *parent = nullptr);

    QQuickItem* flickable() const
    {
        return m_flickable;
    }

    Q_INVOKABLE qreal verticalExtent() const;

    Q_INVOKABLE qreal minYExtent() const;

signals:

    void flickableChanged(QQuickItem* flickable);

public slots:

void setFlickable(QQuickItem* flickable);

private:
    QQuickItem* m_flickable = nullptr;
};

#endif // FLICKABLEEXTENTQUERY_HXX
