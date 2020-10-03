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

#pragma once

#include <string>

#include <QObject>
#include <QByteArray>
#include <QUrl>

class UpdateChecker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Status status READ status  NOTIFY statusChanged)
    Q_PROPERTY(QUrl updateUri READ updateUri NOTIFY statusChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY statusChanged)
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    enum Status {
        NoUpdate,
        PointUpdate,
        MajorUpdate
    };

    Q_ENUMS(Status)


    Status status() const
    {
        return m_status;
    }

    QUrl updateUri() const
    {
        return m_updateUri;
    }

    QString updateVersion() const
    {
        return _currentUpdateVersion;
    }

signals:

    void statusChanged(Status status);

public slots:
    void ignoreUpdate();

private slots:
    void receivedUpdateXML(QByteArray body);

private:
    std::string _majorMinorVersion; ///< version without patch level

    QString _currentUpdateVersion;

    Status m_status = NoUpdate;
    QUrl m_updateUri;
};

