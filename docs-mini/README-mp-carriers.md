# MP carriers

[TOC]

## Overview

There are two settings properties that improve the behaviour of MP carriers.

Both appear in the "Multiplayer Settings" dialogue, from menu "Multiplayer/Multiplayer Settings".

* `/sim/mp-carriers/auto-attach`

    This is true by default.
    
    When an MP carriers appear, we automatically enable the matching AI scenario so that it will
    be visible to the user.

* `/sim/mp-carriers/latch-always`

    This is not enabled by default.
    
    When enabled we force an AI carrier to exactly follow the position and orientation of the MP carrier; this is done by C++ in each frame (otherwise Flightgear uses Nasal to periodically change the AI carrier's velocity to make it approximately follow the MP carrier).
    
    This works better as long as multiplayer motion is smooth (e.g. with simple-time).
    
    And it also improves behaviour when replaying recordings because the carrier position is replayed exactly the same as when recorded.
