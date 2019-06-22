//
//  QmlPropertyModel.hxx
//
//  Created by James Turner on 16/06/2019.
//

#ifndef QmlPropertyModel_hpp
#define QmlPropertyModel_hpp

#include <QAbstractListModel>

class FGQmlPropertyModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged);
    Q_PROPERTY(QString childName READ childName WRITE setChildName NOTIFY childNameChanged);

public:
    FGQmlPropertyModel(QObject* parent = nullptr);
    ~FGQmlPropertyModel() override;
    QString rootPath() const;

    QString childName() const;

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex& m, int role) const override;
public slots:
    void setRootPath(QString rootPath);

    void setChildName(QString childName);

signals:
    void rootPathChanged(QString rootPath);

    void childNameChanged(QString childName);

private:
    class PropertyModelPrivate;
    std::unique_ptr<PropertyModelPrivate> d;
};

#endif /* QmlPropertyModel_hpp */
