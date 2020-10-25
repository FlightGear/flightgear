// Written by James Turner, started October 2020
//
// Copyright (C) 2020 James Turner <james@flightgear.org>
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

#include "LauncherNotificationsController.hxx"

#include <QAbstractListModel>
#include <QDebug>
#include <QQmlEngine>
#include <QSettings>

static LauncherNotificationsController* static_instance = nullptr;

namespace {

const int IdRole = Qt::UserRole + 1;
const int SourceRole = Qt::UserRole + 2;
const int ArgsRole = Qt::UserRole + 3;

} // namespace

class LauncherNotificationsController::NotificationsModel : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex&) const override
    {
        return static_cast<int>(_data.size());
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        const int row = index.row();
        if ((row < 0) || (row >= static_cast<int>(_data.size()))) {
            return {};
        }

        const auto& d = _data.at(row);
        switch (role) {
        case IdRole: return d.id;
        case SourceRole: return d.source;
        case ArgsRole: return QVariant::fromValue(d.args);
        default:
            break;
        }

        return {};
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> result = QAbstractListModel::roleNames();
        result[IdRole] = "id";
        result[SourceRole] = "source";
        result[ArgsRole] = "args";
        return result;
    }

    void removeIndex(int index)
    {
        beginRemoveRows({}, index, index);
        _data.erase(_data.begin() + index);
        endRemoveRows();
    }

    void append(QString id, QUrl source, QJSValue args)
    {
        const int newRow = static_cast<int>(_data.size());
        beginInsertRows({}, newRow, newRow);
        _data.push_back({id, source, args});
        endInsertRows();
    }

    struct Data {
        QString id;
        QUrl source;
        QJSValue args;
    };

    std::vector<Data> _data;
};

LauncherNotificationsController::LauncherNotificationsController(QObject* pr, QQmlEngine* engine) : QObject(pr)
{
    Q_ASSERT(static_instance == nullptr);
    static_instance = this;

    _model = new NotificationsModel;

    _qmlEngine = engine;
}

LauncherNotificationsController::~LauncherNotificationsController()
{
    static_instance = nullptr;
}

LauncherNotificationsController* LauncherNotificationsController::instance()
{
    return static_instance;
}

QAbstractItemModel* LauncherNotificationsController::notifications() const
{
    return _model;
}

QJSValue LauncherNotificationsController::argsForIndex(int index) const
{
    if ((index < 0) || (index >= static_cast<int>(_model->_data.size()))) {
        return {};
    }
    const auto& d = _model->_data.at(index);
    qDebug() << Q_FUNC_INFO << index;
    return d.args;
}

QJSEngine* LauncherNotificationsController::jsEngine()
{
    return _qmlEngine;
}

void LauncherNotificationsController::dismissIndex(int index)
{
    const auto& d = _model->_data.at(index);

    // if the notificsation supports persistent dismissal, then record this
    // fact in the global settings, so we don't show it again.
    // restore defaults will of course clear these settings, but that's
    // desirable anyway.
    if (d.args.property("persistent-dismiss").toBool()) {
        QSettings settings;
        settings.beginGroup("dismissed-notifications");
        settings.setValue(d.id, true);
    }

    _model->removeIndex(index);
}

void LauncherNotificationsController::postNotification(QString id, QUrl source, QJSValue args)
{
    const bool supportsPersistentDismiss = args.property("persistent-dismiss").toBool();
    if (supportsPersistentDismiss) {
        QSettings settings;
        settings.beginGroup("dismissed-notifications");
        bool alreadyDimissed = settings.value(id).toBool();
        if (alreadyDimissed) {
            qWarning() << "Skipping notification" << id << ", was previousl dimissed by user";
            return;
        }
    }

    _model->append(id, source, args);
}
