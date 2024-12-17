/*
 * SPDX-FileCopyrightText: Copyright (C) 2024  James Turner - james@flightgear.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


#include <QSettings>
#include <memory>

namespace flightgear {
/// @brief  used to create / access the settings file for this version
/// Prefer over the default QSettings ctor since it references the trunk
/// version
QSettings getQSettings();


std::unique_ptr<QSettings> createQSettings();
} // namespace flightgear
