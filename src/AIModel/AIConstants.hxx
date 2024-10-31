/*
 * SPDX-FileName: AIConstants.hxx
 * SPDX-FileComment: AIConstants
 * SPDX-FileCopyrightText: Copyright (C) 2024 Keith Paterson - keith.paterson@gmx.de
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

namespace AILeg {
enum Type {
    STARTUP_PUSHBACK = 1,
    TAXI = 2,
    TAKEOFF = 3,
    CLIMB = 4,
    CRUISE = 5,
    APPROACH = 6,
    HOLD = 7,
    LANDING = 8,
    PARKING_TAXI = 9,
    PARKING = 10
};
}

// 1 = joined departure queue; 2 = Passed DepartureHold waypoint; handover control to tower; 0 = any other state.
namespace AITakeOffStatus {
enum Type {
    NONE = 0,
    QUEUED = 1,             // joined departure queue
    CLEARED_FOR_TAKEOFF = 2 // Passed DepartureHold waypoint; handover control to tower;
};
}
