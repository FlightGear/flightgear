#include "FlickableExtentQuery.hxx"

#include <QtQuick/private/qquickflickable_p.h>

class FriendFlickable : public QQuickFlickable
{
public:
    friend class FlickableExtentQuery;
};

FlickableExtentQuery::FlickableExtentQuery(QObject *parent) : QObject(parent)
{

}

qreal FlickableExtentQuery::verticalExtent() const
{
    QQuickFlickable* flick = qobject_cast<QQuickFlickable*>(m_flickable);
    if (!flick) {
        return 0;
    }

    FriendFlickable* ff = static_cast<FriendFlickable*>(flick);
    qreal extent = -ff->maxYExtent() + ff->minYExtent();
    return extent;
}

qreal FlickableExtentQuery::minYExtent() const
{
    QQuickFlickable* flick = qobject_cast<QQuickFlickable*>(m_flickable);
    if (!flick) {
        return 0;
    }

    FriendFlickable* ff = static_cast<FriendFlickable*>(flick);
    return ff->minYExtent();
}

void FlickableExtentQuery::setFlickable(QQuickItem *flickable)
{
    if (m_flickable == flickable)
        return;

    m_flickable = flickable;
    emit flickableChanged(m_flickable);
}
