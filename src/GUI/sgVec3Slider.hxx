#ifndef _VEC3_SLIDER_H
#define  _VEC3_SLIDER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <stdio.h>
#include "gui.h"

void PilotOffsetInit();
void PilotOffsetInit( sgVec3 vec );
void PilotOffsetAdjust( puObject * );
void Vec3FromHeadingPitchRadius( sgVec3 vec3, float heading, float pitch, float radius );
//void PilotOffsetGet( float *po );
sgVec3 *PilotOffsetGet();
void PilotOffsetSet( int opt, float setting);
float PilotOffsetGetSetting( int opt );

#endif // _VEC3_SLIDER_H

/* binding functions for chase view offset */

static void
setPilotOffsetHeadingDeg (float value)
{
	PilotOffsetSet(0, value);
}

static float
getPilotOffsetHeadingDeg ()
{
	return( PilotOffsetGetSetting(0) );
}


static void
setPilotOffsetPitchDeg (float value)
{
	PilotOffsetSet(1, value);
}

static float
getPilotOffsetPitchDeg ()
{
	return( PilotOffsetGetSetting(1) );
}


static void
setPilotOffsetRadiusM (float value)
{
	PilotOffsetSet(2, value);
}

static float
getPilotOffsetRadiusM ()
{
	return( PilotOffsetGetSetting(2) );
}

