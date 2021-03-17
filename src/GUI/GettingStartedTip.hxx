#ifndef GETTINGSTARTEDTIP_HXX
#define GETTINGSTARTEDTIP_HXX

#include <QQuickItem>
#include <QString>
#include <QPointer>

#include "GettingStartedTipsController.hxx"

class GettingStartedTip : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QString tipId MEMBER _id NOTIFY tipChanged)
    Q_PROPERTY(QString text MEMBER _text NOTIFY tipChanged)
    Q_PROPERTY(QString nextTip MEMBER _nextId NOTIFY tipChanged)

    Q_PROPERTY(Arrow arrow MEMBER _arrow NOTIFY tipChanged)

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

    Q_PROPERTY(GettingStartedTipsController* controller READ controller NOTIFY controllerChanged)

    /// standalone tips are excluded from their scope when activated;
    /// instead they need to be activated manually
    Q_PROPERTY(bool standalone READ standalone WRITE setStandalone NOTIFY standaloneChanged)
public:
    enum class Arrow
    {
        TopCenter,  // directly below the item, centered
        LeftCenter,
        RightCenter,
        BottomCenter,
        BottomRight,
        TopRight,
        TopLeft,
        LeftTop, // on the left side, at the top
        NoArrow
    };

    Q_ENUM(Arrow)

    explicit GettingStartedTip(QQuickItem *parent = nullptr);
    ~GettingStartedTip() override;

    QString tipId() const
    {
        return _id;
    }

    Arrow arrow() const
    {
        return _arrow;
    }

    bool isEnabled() const
    {
        return _enabled;
    }

    GettingStartedTipsController* controller() const;

    void componentComplete() override; // from QQmlParserStatus

    bool standalone() const
    {
        return _standalone;
    }

    QString nextTip() const
    {
        return _nextId;
    }

    // allow disabling all tips progrmatically : this is a temporary
    // measure to make life less annoying for our translators
    static void setGlobalTipsEnabled(bool enable);
public slots:
    void setEnabled(bool enabled);

    void showOneShot();
    void setStandalone(bool standalone);

signals:

    void tipChanged();
    void enabledChanged();
    void controllerChanged();

    void standaloneChanged(bool standalone);

protected:
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value);

private:
    void registerWithScope();
    void unregisterFromScope();

    GettingStartedTipsController* findController();

    QString _id,
        _text,
        _nextId;

    Arrow _arrow = Arrow::LeftCenter;
    bool _enabled = true;
    QPointer<GettingStartedTipsController> _controller;
    bool _standalone = false;
};

#endif // GETTINGSTARTEDTIP_HXX
