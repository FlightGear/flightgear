/*
 * SPDX-FileName: QmlPositionedModel.hxx
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: Copyright (C) 2024 James Turner
 */

#pragma once

#include <QAbstractListModel>
#include <memory>

#include <Airports/airports_fwd.hxx>
#include <Navaids/positioned.hxx>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
Q_MOC_INCLUDE("QmlPositioned.hxx")
#endif

class QmlPositioned;

class QmlPositionedModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool empty READ isEmpty NOTIFY sizeChanged)
public:
    QmlPositionedModel(QObject* parent = nullptr);
    ~QmlPositionedModel() override;

    void setValues(const FGPositionedList& posItems);

    void setValues(const FGRunwayList& runways);

    void setValues(const FGParkingList& runways);

    void clear();

    int rowCount(const QModelIndex& parent) const override;

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex& m, int role) const override;

    Q_INVOKABLE int indexOf(QmlPositioned* pos) const;

    Q_INVOKABLE QmlPositioned* itemAt(int index) const;

    bool isEmpty() const;
signals:
    void sizeChanged();

private:
    class QmlPositionedModelPrivate;
    std::unique_ptr<QmlPositionedModelPrivate> d;
};
