#include "GettingStartedTip.hxx"

#include "GettingStartedScope.hxx"
#include "GettingStartedTipsController.hxx"

static bool static_globalEnableTips = true;

GettingStartedTip::GettingStartedTip(QQuickItem *parent) :
    QQuickItem(parent)
{
    _enabled = static_globalEnableTips;

    setImplicitHeight(1);
    setImplicitWidth(1);

    if (_enabled) {
        registerWithScope();
    }
}

GettingStartedTip::~GettingStartedTip()
{
    if (_enabled) {
        unregisterFromScope();
    }
}

GettingStartedTipsController *GettingStartedTip::controller() const
{
    return _controller.data();
}

void GettingStartedTip::componentComplete()
{
    if (_enabled && !_controller) {
        registerWithScope();
    }

    QQuickItem::componentComplete();
}

void GettingStartedTip::setGlobalTipsEnabled(bool enable)
{
    static_globalEnableTips = enable;
}

void GettingStartedTip::setEnabled(bool enabled)
{
    if (_enabled == enabled)
        return;

    _enabled = enabled & static_globalEnableTips;
    if (enabled) {
        registerWithScope();
    } else {
        unregisterFromScope();
    }

    emit enabledChanged();
}

void GettingStartedTip::showOneShot()
{
    auto ctl = findController();
    if (ctl) {
        ctl->showOneShotTip(this);
    }
}

void GettingStartedTip::setStandalone(bool standalone)
{
    if (_standalone == standalone)
        return;

    _standalone = standalone;
    emit standaloneChanged(_standalone);

    // re-register with our scope, which will also unregister us if
    // necessary.
    if (_enabled) {
        registerWithScope();
    }
}

void GettingStartedTip::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    const bool isParentChanged = (change == ItemParentHasChanged);
    if (isParentChanged && _enabled) {
        unregisterFromScope();
    }

    QQuickItem::itemChange(change, value);

    if (isParentChanged && _enabled) {
        registerWithScope();
    }
}

void GettingStartedTip::registerWithScope()
{
    if (_controller) {
        unregisterFromScope();
    }

    if (_standalone) {
        return; // standalone tips don't register
    }

    auto ctl = findController();
    if (!ctl) {
        return;
    }

    bool ok = ctl->addTip(this);
    if (ok) {
        _controller = ctl;
        emit controllerChanged();
    }
}

GettingStartedTipsController* GettingStartedTip::findController()
{
    QQuickItem* pr = const_cast<QQuickItem*>(parentItem());
    if (!pr) {
        return nullptr;
    }

    while (pr) {
        auto sa = qobject_cast<GettingStartedScopeAttached*>(qmlAttachedPropertiesObject<GettingStartedScope>(pr, false));
        if (sa && sa->controller()) {
            return sa->controller();
        }

        pr = pr->parentItem();
    }

    return nullptr;
}

void GettingStartedTip::unregisterFromScope()
{
    if (_controller) {
        _controller->removeTip(this);
        _controller.clear();
        emit controllerChanged();
    }
}
