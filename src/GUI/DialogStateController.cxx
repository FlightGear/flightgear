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

#include "config.h"

#include "DialogStateController.hxx"

#include <QDebug>

void DialogStateController::add(QString key, QJSValue cb)
{
    bool wasEmpty = m_changes.isEmpty();
    if (!cb.isCallable()) {
        qWarning() << "tried to insert non-callable into state key:" << key;
        return;
    }

    m_changes.insert(key, cb);

    if (wasEmpty) {
        emit canApplyChanged(true);
    }
}

void DialogStateController::apply()
{
    auto it = m_changes.begin();
    for (; it != m_changes.end(); ++it) {
        QJSValueList args;
        args.append(QJSValue(it.key()));
        it.value().call(args);
    }
}

void DialogStateController::cancel()
{
    m_changes.clear();
    emit canApplyChanged(false);
}

void DialogStateController::restoreDefaults()
{
}

void DialogStateController::setCanRestoreDefaults(bool canRestoreDefaults)
{
    if (m_canRestoreDefaults == canRestoreDefaults)
        return;

    m_canRestoreDefaults = canRestoreDefaults;
    emit canRestoreDefaultsChanged(m_canRestoreDefaults);
}
