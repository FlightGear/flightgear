#include "GettingStartedScope.hxx"

#include "GettingStartedTipsController.hxx"


GettingStartedScope::GettingStartedScope(QObject *parent) : QObject(parent)
{

}

GettingStartedScopeAttached *GettingStartedScope::qmlAttachedProperties(QObject *object)
{
    auto c = new GettingStartedScopeAttached(object);
    return c;
}

GettingStartedScopeAttached::GettingStartedScopeAttached(QObject *parent) : QObject(parent)
{

}

void GettingStartedScopeAttached::setController(GettingStartedTipsController *controller)
{
    if (_controller == controller)
        return;

    _controller = controller;
    emit controllerChanged();
}
