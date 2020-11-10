#ifndef GETTINGSTARTEDTIPSCONTROLLER_HXX
#define GETTINGSTARTEDTIPSCONTROLLER_HXX

#include <QObject>
#include <QVector>
#include <QQuickItem>
#include <QPointer>

// forward decls
class GettingStartedTip;

/**
 * @brief Manage presentation of getting started tips on a screen/page
 */
class GettingStartedTipsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString scopeId READ scopeId WRITE setScopeId NOTIFY scopeIdChanged)

    Q_PROPERTY(GettingStartedTip* tip READ tip NOTIFY tipChanged)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)

    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)

    Q_PROPERTY(QPointF tipPositionInVisualArea READ tipPositionInVisualArea NOTIFY tipPositionInVisualAreaChanged)
    Q_PROPERTY(bool tipPositionValid READ tipPositionValid NOTIFY tipPositionInVisualAreaChanged)

    Q_PROPERTY(QRectF tipGeometry READ tipGeometry NOTIFY tipGeometryChanged)
    Q_PROPERTY(int activeTipHeight READ activeTipHeight WRITE setActiveTipHeight NOTIFY activeTipHeightChanged)
    Q_PROPERTY(QRectF contentGeometry READ contentGeometry NOTIFY tipChanged)

    Q_PROPERTY(QQuickItem* visualArea READ visualArea WRITE setVisualArea NOTIFY visualAreaChanged)

public:
    explicit GettingStartedTipsController(QObject *parent = nullptr);
    ~GettingStartedTipsController();

    int count() const;

    int index() const;

    GettingStartedTip* tip() const;

    QString scopeId() const
    {
        return _scopeId;
    }

    bool isActive() const;

    QPointF tipPositionInVisualArea() const;

    QQuickItem* visualArea() const
    {
        return _visualArea;
    }

    QRectF tipGeometry() const;

    bool tipPositionValid() const;

    int activeTipHeight() const;

    /**
     * @brief contentGeometry - based on the active tip, return the box
     * (relative to the total tipGeometry) which content should occupy.
     * This allows for offseting due to the arrow position, and also specifies
     * the width for computing wrapped text height
     *
     * The actual height is the maximum height; the computed value should be
     * set by activeTipHeight
     */
    QRectF contentGeometry() const;

public slots:

    void close();

    void setIndex(int index);

    void setScopeId(QString scopeId);

    void setVisualArea(QQuickItem* visualArea);

    void setActiveTipHeight(int activeTipHeight);

    /**
     * @brief showOneShotTip - show a single tip on its own, if it has not
     * previously been shown before.
     *
     * This is used for pieces of UI which are not always present; the first
     * time the UI is displayed to the user, we can show a tip describing it.
     * Once the user closes the tip, it will not reappear.
     *
     * The tip will be activated if the controller is currently inactive.
     * If the controller was alreayd active, the tip will be shown, and when
     * closed, the other tips will be shown. If the one-shot tip is part of
     * the currently active list, this actually does nothing, to avoid showing
     * the same tip twice
     */
    void showOneShotTip(GettingStartedTip* tip);

    void tipsWereReset();
signals:

    void countChanged(int count);
    void indexChanged(int index);
    void tipChanged();
    void scopeChanged(QObject* scope);
    void scopeIdChanged(QString scopeId);
    void activeChanged();
    void tipGeometryChanged();

    void tipPositionInVisualAreaChanged();

    void visualAreaChanged(QQuickItem* visualArea);

    void activeTipHeightChanged(int activeTipHeight);

private:
    friend class GettingStartedTip;

    bool shouldShowScope() const;

    void currentTipUpdated();

    bool addTip(GettingStartedTip* t);
    void removeTip(GettingStartedTip* t);

    void onOneShotDestroyed();
private:

    class ItemPositionObserver;

    bool _scopeActive = false;
    int _index = 0;
    QString _scopeId;
    ItemPositionObserver* _positionObserver = nullptr;
    ItemPositionObserver* _viewAreaObserver = nullptr;

    QPointer<GettingStartedTip> _oneShotTip;
    QVector<GettingStartedTip*> _tips;
    QQuickItem* _visualArea = nullptr;
    int _activeTipHeight = 0;
};

#endif // GETTINGSTARTEDTIPSCONTROLLER_HXX
