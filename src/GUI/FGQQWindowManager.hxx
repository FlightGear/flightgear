#ifndef FGQQWINDOWMANAGER_HXX
#define FGQQWINDOWMANAGER_HXX

#include <QObject>
#include <memory>

// forward decls
class QAbstractItemModel;
class QQmlEngine;

class FGQQWindowManager : public QObject
{
    Q_OBJECT
public:
    explicit FGQQWindowManager(QQmlEngine* engine, QObject* parent = nullptr);
    ~FGQQWindowManager();

    Q_PROPERTY(QAbstractItemModel* windows READ windows CONSTANT)

    QAbstractItemModel* windows() const;

    Q_INVOKABLE bool show(QString windowId);

    Q_INVOKABLE bool requestPopout(QString windowId);

    Q_INVOKABLE bool requestClose(QString windowId);

    Q_INVOKABLE bool requestPopin(QString windowId);
signals:

public slots:

private:
    class WindowManagerPrivate;

    std::unique_ptr<WindowManagerPrivate> _d;
    QQmlEngine* _engine = nullptr;
};

#endif // FGQQWINDOWMANAGER_HXX
