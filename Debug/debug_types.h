// NB: To add a dbg_class, add it here, and add it to the structure in
// fg_debug.c

typedef enum {
    FG_NONE      = 0x00000000,

    FG_TERRAIN   = 0x00000001,
    FG_ASTRO     = 0x00000002,
    FG_FLIGHT    = 0x00000004,
    FG_INPUT     = 0x00000008,
    FG_GL        = 0x00000010,
    FG_VIEW      = 0x00000020,
    FG_COCKPIT   = 0x00000040,
    FG_GENERAL   = 0x00000080,
    FG_MATH      = 0x00000100,
    FG_EVENT     = 0x00000200,
    FG_AIRCRAFT  = 0x00000400,
    FG_AUTOPILOT = 0x00000800,
    FG_SERIAL    = 0x00001000,
    FG_UNDEFD    = 0x00002000, // For range checking

    FG_ALL     = 0xFFFFFFFF
} fgDebugClass;


// NB: To add a priority, add it here.
typedef enum {
    FG_BULK,	    // For frequent messages
    FG_DEBUG, 	    // Less frequent debug type messages
    FG_INFO,        // Informatory messages
    FG_WARN,	    // Possible impending problem
    FG_ALERT       // Very possible impending problem
    // FG_EXIT,        // Problem (no core)
    // FG_ABORT        // Abandon ship (core)
} fgDebugPriority;

