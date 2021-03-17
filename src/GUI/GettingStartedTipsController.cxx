#include "GettingStartedTipsController.hxx"

#include <algorithm>

#include <QSettings>
#include <QDebug>
#include <QQmlContext>
#include <QTimer>

#include <QtQml> // qmlContext

#include "GettingStartedTip.hxx"
#include "TipBackgroundBox.hxx"

struct TipGeometryByArrowLocation
{
    TipGeometryByArrowLocation(GettingStartedTip::Arrow a, const QRectF& g, Qt::Alignment al) :
        arrow(a),
        geometry(g),
        verticalAlignment(al)
    {
    }

    GettingStartedTip::Arrow arrow;
    QRectF geometry;
    // specify how vertical space is adjusted; is the top, bottom or center fixed
    Qt::Alignment verticalAlignment = Qt::AlignVCenter;
};

const double tipBoxWidth = 300.0;
const double halfBoxWidth = tipBoxWidth * 0.5;
const double arrowSideOffset = TipBackgroundBox::arrowSideOffset();
const double rightSideOffset = -tipBoxWidth + arrowSideOffset;
const double dummyHeight = 200.0;
const double topHeightOffset = -TipBackgroundBox::arrowHeight();

static std::initializer_list<TipGeometryByArrowLocation> static_tipGeometries = {
    {GettingStartedTip::Arrow::BottomRight, QRectF{rightSideOffset, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignBottom},
    {GettingStartedTip::Arrow::BottomCenter, QRectF{halfBoxWidth, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignBottom},
    {GettingStartedTip::Arrow::TopCenter, QRectF{-halfBoxWidth, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignTop},
    {GettingStartedTip::Arrow::TopRight, QRectF{rightSideOffset, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignTop},
    {GettingStartedTip::Arrow::TopLeft, QRectF{-arrowSideOffset, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignTop},
    {GettingStartedTip::Arrow::LeftCenter, QRectF{0.0, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignVCenter},
    {GettingStartedTip::Arrow::RightCenter, QRectF{-(tipBoxWidth + TipBackgroundBox::arrowHeight()), 0.0, tipBoxWidth, dummyHeight}, Qt::AlignVCenter},
    {GettingStartedTip::Arrow::LeftTop, QRectF{0.0, topHeightOffset, tipBoxWidth, dummyHeight}, Qt::AlignTop},
    {GettingStartedTip::Arrow::NoArrow, QRectF{-halfBoxWidth, 0.0, tipBoxWidth, dummyHeight}, Qt::AlignVCenter},

};

/**
 * @brief The GettingStartedTipsController::ItemPositionObserver class
 *
 * This is a helper to observe the full position (and in the future, transform if required)
 * of a QQuickItem, so we can update a signal when the on-screen position changes. This
 * is necessary to re-transform the tooltip location if the item it's 'attached' to
 * moves, or some ancestor does.
 *
 * At present this does not handle arbitrary scaling or rotation, but observing those
 * signals would also be possible.
 */
class GettingStartedTipsController::ItemPositionObserver : public QObject
{
    Q_OBJECT
public:
    ItemPositionObserver(QObject* pr) :
        QObject(pr)
    {
        _notMovingTimeout = new QTimer(this);
        _notMovingTimeout->setSingleShot(true);
        _notMovingTimeout->setInterval(1000);

        connect(this, SIGNAL(itemPositionChanged()),
                _notMovingTimeout, SLOT(start()));

        connect(_notMovingTimeout, &QTimer::timeout,
                this, &ItemPositionObserver::itemNotMoving);
    }

    void setObservedItem(QQuickItem* obs)
    {
        if (obs == _observedItem)
            return;

        if (_observedItem) {
            for (auto o = _observedItem; o; o = o->parentItem()) {
                disconnect(o, nullptr, this, nullptr);
            }
        }

        _observedItem = obs;
        if (obs) {
            startObserving(_observedItem);
        }
    }

    bool hasRecentlyMoved() const
    {
        return _notMovingTimeout->isActive();
    }
signals:
    void itemPositionChanged();

    void itemNotMoving();
private:
    void startObserving(QQuickItem* obs)
    {

        connect(obs, &QQuickItem::xChanged, this, &ItemPositionObserver::itemPositionChanged);
        connect(obs, &QQuickItem::yChanged, this, &ItemPositionObserver::itemPositionChanged);
        connect(obs, &QQuickItem::widthChanged, this, &ItemPositionObserver::itemPositionChanged);
        connect(obs, &QQuickItem::heightChanged, this, &ItemPositionObserver::itemPositionChanged);

        // recurse up the item hierarchy
        if (obs->parentItem()) {
            startObserving(obs->parentItem());
        }
    }

    QPointer<QQuickItem> _observedItem;
    QTimer* _notMovingTimeout = nullptr;
};

// static used to ensure only one controller is active at a time
static QPointer<GettingStartedTipsController> static_activeController;

GettingStartedTipsController::GettingStartedTipsController(QObject *parent) : QObject(parent)
{
    // observer for the tip item
    _positionObserver = new ItemPositionObserver(this);
    connect(_positionObserver, &ItemPositionObserver::itemPositionChanged,
            this, &GettingStartedTipsController::tipPositionInVisualAreaChanged);

    // observer for the visual area (which could also be scrolled)
    _viewAreaObserver = new ItemPositionObserver(this);
    connect(_viewAreaObserver, &ItemPositionObserver::itemPositionChanged,
            this, &GettingStartedTipsController::tipPositionInVisualAreaChanged);

    connect(_positionObserver, &ItemPositionObserver::itemNotMoving,
            this, &GettingStartedTipsController::tipPositionInVisualAreaChanged);
    connect(_viewAreaObserver, &ItemPositionObserver::itemNotMoving,
            this, &GettingStartedTipsController::tipPositionInVisualAreaChanged);

    auto qqParent = qobject_cast<QQuickItem*>(parent);
    if (qqParent) {
        setVisualArea(qqParent);
    }
}

GettingStartedTipsController::~GettingStartedTipsController()
{
}

int GettingStartedTipsController::count() const
{
    if (_oneShotTip) {
        return 1;
    }

    return _tips.size();
}

int GettingStartedTipsController::index() const
{
    if (_oneShotTip) {
        return 0;
    }

    return _index;
}

GettingStartedTip *GettingStartedTipsController::tip() const
{
    if (_oneShotTip) {
        return _oneShotTip;
    }

    if (_tips.empty())
        return nullptr;

    if ((_index < 0) || (_index >= _tips.size()))
        return nullptr;

    return _tips.at(_index);
}

void GettingStartedTipsController::setVisualArea(QQuickItem *visualArea)
{
    if (_visualArea == visualArea)
        return;

    _visualArea = visualArea;
    _viewAreaObserver->setObservedItem(_visualArea);

    emit tipPositionInVisualAreaChanged();
    emit visualAreaChanged(_visualArea);
}

void GettingStartedTipsController::setActiveTipHeight(int activeTipHeight)
{
    if (_activeTipHeight == activeTipHeight)
        return;

    _activeTipHeight = activeTipHeight;
    emit activeTipHeightChanged(_activeTipHeight);
    emit tipGeometryChanged();
}

void GettingStartedTipsController::showOneShotTip(GettingStartedTip *tip)
{
    if (_scopeActive) {
        return;
    }

    if (static_activeController != this) {
        return;
    }

    QSettings settings;
    settings.beginGroup("GettingStarted-DontShow");
    if (settings.value(tip->tipId()).toBool()) {
        return;
    }

    // mark the tip as shown
    settings.setValue(tip->tipId(), true);
    _oneShotTip = tip;

    connect(_oneShotTip, &QObject::destroyed, this, &GettingStartedTipsController::onOneShotDestroyed);

    currentTipUpdated();
    emit indexChanged(0);
    emit countChanged(count());
}

void GettingStartedTipsController::tipsWereReset()
{
    bool a = shouldShowScope();
    if (a != _scopeActive) {
        _scopeActive = a;
        static_activeController = this; // we became active
        emit activeChanged();
        currentTipUpdated();
    }
}

void GettingStartedTipsController::currentTipUpdated()
{
    _positionObserver->setObservedItem(tip());

    emit activeChanged();
    emit tipChanged();
    emit tipPositionInVisualAreaChanged();
    emit tipGeometryChanged();
}

bool GettingStartedTipsController::addTip(GettingStartedTip *t)
{
    if (_tips.contains(t)) {
        qWarning() << Q_FUNC_INFO << "Duplicate tip" << t;
        return false;
    }

    // this logic is important to suppress duplicate tips inside a ListView or Repeater;
    // effectively, we only show a tip on the first registered instance.
    Q_FOREACH(GettingStartedTip* tip, _tips) {
        if (tip->tipId() == t->tipId()) {
            return false;
        }
    }

    _tips.append(t);
    // order tips by nextTip ID, if defined
    std::sort(_tips.begin(), _tips.end(), [](const GettingStartedTip* a, GettingStartedTip *b) {
       return a->nextTip() == b->tipId();
    });

    currentTipUpdated();
    emit countChanged(count());
    return true;
}

void GettingStartedTipsController::removeTip(GettingStartedTip *t)
{
    const bool removedActive = (tip() == t);
    if (!_tips.removeOne(t)) {
        qWarning() << Q_FUNC_INFO << "tip not found";
    }

    if (removedActive) {
        _index = qMax(_index - 1, 0);
    }

    currentTipUpdated();
    emit countChanged(count());
}

void GettingStartedTipsController::onOneShotDestroyed()
{
    if (_oneShotTip == sender()) {
        emit activeChanged();
        currentTipUpdated();
    }
}

bool GettingStartedTipsController::isActive() const
{
    if (_oneShotTip)
        return true;

    return _scopeActive && !_tips.empty();
}

QPointF GettingStartedTipsController::tipPositionInVisualArea() const
{
    auto t = tip();
    if (!_visualArea || !t) {
        return {};
    }

    return _visualArea->mapFromItem(t, QPointF{0,0});
}

QRectF GettingStartedTipsController::tipGeometry() const
{
    auto t = tip();
    if (!t)
        return {};

    const auto arrow = t->arrow();
    auto it = std::find_if(static_tipGeometries.begin(), static_tipGeometries.end(),
                           [arrow](const TipGeometryByArrowLocation& tg)
    {
        return tg.arrow == arrow;
    });

    if (it == static_tipGeometries.end()) {
        qWarning() << Q_FUNC_INFO << "Missing tip geometry" << arrow;
        return {};
    }

    QRectF g = it->geometry;
    if ((arrow == GettingStartedTip::Arrow::LeftCenter) 
            || (arrow == GettingStartedTip::Arrow::RightCenter)
            || (arrow == GettingStartedTip::Arrow::LeftTop)
            || (arrow == GettingStartedTip::Arrow::NoArrow))
    {
        g.setHeight(_activeTipHeight);
    } else {
        g.setHeight(_activeTipHeight + TipBackgroundBox::arrowHeight());
    }

    switch (it->verticalAlignment) {
    case Qt::AlignBottom:
        g.moveBottom(0);
        break;

    case Qt::AlignTop:
        g.moveTop(0);
        break;

    case Qt::AlignVCenter:
        g.moveTop(_activeTipHeight * -0.5);
        break;
    }

    return g;
}

bool GettingStartedTipsController::tipPositionValid() const
{
    if (!_visualArea || !isActive())
        return false;

    // hide tips when resizing the window or scrolling; it's visually distracting otherwise
    if (_positionObserver->hasRecentlyMoved() || _viewAreaObserver->hasRecentlyMoved()) {
        return false;
    }

    if (_oneShotTip)
        return true;

    return !_tips.empty();
}

int GettingStartedTipsController::activeTipHeight() const
{
    return _activeTipHeight;
}

QRectF GettingStartedTipsController::contentGeometry() const
{
    QRectF g(0.0, 0.0, tipBoxWidth, 200.0);

    auto t = tip();
    if (!t)
        return g;

    const auto arrow = t->arrow();
    if ((arrow == GettingStartedTip::Arrow::TopCenter) ||
            (arrow == GettingStartedTip::Arrow::TopLeft) ||
            (arrow == GettingStartedTip::Arrow::TopRight))
    {
        g.moveTop(TipBackgroundBox::arrowHeight());
    }

    if (arrow == GettingStartedTip::Arrow::RightCenter) {
        g.setWidth(tipBoxWidth - TipBackgroundBox::arrowHeight());
    }

    if (arrow == GettingStartedTip::Arrow::LeftCenter) {
        g.setWidth(tipBoxWidth - TipBackgroundBox::arrowHeight());
        g.moveLeft(TipBackgroundBox::arrowHeight());
    }

    return g;
}

void GettingStartedTipsController::close()
{
    // one-shot tips handle this logic differently; we set the don't show
    // when the tip first appears
    if (_oneShotTip) {
        disconnect(_oneShotTip, nullptr, this, nullptr);
        _oneShotTip = nullptr;
        static_activeController.clear();
    } else {
        if (_scopeActive) {
            static_activeController.clear();
        }

        QSettings settings;
        settings.beginGroup("GettingStarted-DontShow");
        settings.setValue(_scopeId, true);
        _scopeActive = false;
    }

    emit activeChanged();
    currentTipUpdated();
}

void GettingStartedTipsController::setIndex(int index)
{
    if (_oneShotTip) {
        return;
    }

    if (_index == index)
        return;

    _index = qBound(0, index, _tips.size() - 1);
    _positionObserver->setObservedItem(tip());

    emit indexChanged(_index);
    currentTipUpdated();
}

void GettingStartedTipsController::setScopeId(QString scopeId)
{
    if (_scopeId == scopeId)
        return;

    _scopeId = scopeId;
    _scopeActive = shouldShowScope();
    if (_scopeActive) {
        static_activeController = this;
    }
    emit scopeIdChanged(_scopeId);
    emit activeChanged();
}

bool GettingStartedTipsController::shouldShowScope() const
{
    if (static_activeController && (static_activeController != this)) {
        return false;
    }

    if (_scopeId.isEmpty())
        return true;

    QSettings settings;
    settings.beginGroup("GettingStarted-DontShow");
    return settings.value(_scopeId).toBool() == false;
}


#include "GettingStartedTipsController.moc"
