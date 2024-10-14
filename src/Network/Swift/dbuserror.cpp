/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbuserror.h"

namespace flightgear::swift {

CDBusError::CDBusError(const DBusError* error)
    : m_name(error->name), m_message(error->message)
{
}

} // namespace flightgear::swift
