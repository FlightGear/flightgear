#ifndef FGQMLINSTANCE_HXX
#define FGQMLINSTANCE_HXX

#include <QObject>

#include <QJSValue>

// forward decls
class FGQmlPropertyNode;

class FGQmlInstance : public QObject
{
    Q_OBJECT
public:
    explicit FGQmlInstance(QObject *parent = nullptr);

    Q_INVOKABLE bool command(QString name, QJSValue args);

    /**
      * retrieve a property by its global path
      */
    Q_INVOKABLE FGQmlPropertyNode* property(QString path, bool create = false) const;
signals:

public slots:
};

#endif // FGQMLINSTANCE_HXX
