#ifndef MODELDATAEXTRACTOR_HXX
#define MODELDATAEXTRACTOR_HXX

#include <QObject>
#include <QJSValue>
#include <QVariant>

class QAbstractItemModel;

class ModelDataExtractor : public QObject
{
    Q_OBJECT
public:
    explicit ModelDataExtractor(QObject *parent = nullptr);

    Q_PROPERTY(QJSValue model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)

    Q_PROPERTY(QVariant data READ data NOTIFY dataChanged)

    QJSValue model() const
    {
        return m_rawModel;
    }

    int index() const
    {
        return m_index;
    }

    QString role() const
    {
        return m_role;
    }

    QVariant data() const;

signals:

    void modelChanged();
    void indexChanged(int index);
    void roleChanged(QString role);

    void dataChanged();

public slots:
    void setModel(QJSValue model);
    void setIndex(int index);
    void setRole(QString role);

private slots:
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
    void clear();

    QAbstractItemModel* m_model = nullptr;
    QJSValue m_rawModel;
    QStringList m_stringsModel;

    int m_index = 0;
    QString m_role;
};

#endif // MODELDATAEXTRACTOR_HXX
