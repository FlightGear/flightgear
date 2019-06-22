// Copyright (C) 2020  James Turner  <james@flightgear.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef QmlPropertyModel_hpp
#define QmlPropertyModel_hpp

#include <QAbstractListModel>
#include <memory>

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
