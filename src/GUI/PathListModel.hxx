#ifndef PATHLISTMODEL_HXX
#define PATHLISTMODEL_HXX

#include <vector>

#include <QAbstractListModel>

const int PathRole = Qt::UserRole + 1;
const int PathEnabledRole = Qt::UserRole + 2;

class PathListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:

    PathListModel(QObject* pr);
    ~PathListModel() override;

    void loadFromSettings(QString key);
    void saveToSettings(QString key) const;

    int rowCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    QHash<int, QByteArray> roleNames() const override;

    static QStringList readEnabledPaths(QString settingsKey);

    QStringList enabledPaths() const;

    int count();
signals:
    void enabledPathsChanged();
    void countChanged();

public slots:
    void removePath(int index);
    void appendPath(QString path);

    void swapIndices(int indexA, int indexB);

private:
    struct PathEntry {
        QString path;
        bool enabled = true;
    };

    std::vector<PathEntry> mPaths;
};

#endif // PATHLISTMODEL_HXX
