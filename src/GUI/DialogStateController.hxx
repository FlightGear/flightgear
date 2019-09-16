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

#ifndef DialogStateController_h
#define DialogStateController_h

#include <QJSValue>
#include <QMap>
#include <QObject>

class DialogStateController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canRestoreDefaults READ canRestoreDefaults WRITE setCanRestoreDefaults NOTIFY canRestoreDefaultsChanged)
    Q_PROPERTY(bool canApply READ canApply NOTIFY canApplyChanged)
public:
    Q_INVOKABLE void add(QString key, QJSValue cb);

    bool canApply() const
    {
        return m_canApply;
    }

    bool canRestoreDefaults() const
    {
        return m_canRestoreDefaults;
    }

public slots:

    void apply();

    void cancel();

    void restoreDefaults();

    void setCanRestoreDefaults(bool canRestoreDefaults);

signals:
    void canApplyChanged(bool canApply);

    void canRestoreDefaultsChanged(bool canRestoreDefaults);

private:
    // list of tracked changes
    QMap<QString, QJSValue> m_changes;

    bool m_canApply = false;
    bool m_canRestoreDefaults = false;
};

#endif /* DialogStateController_h */
