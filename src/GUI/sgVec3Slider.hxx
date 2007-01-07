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
void Vec3FromHeadingPitchRadius( sgVec3 vec3, float heading, float pitch, float radius );
void HeadingPitchRadiusFromVec3( sgVec3 hpr, sgVec3 vec3 );
//void PilotOffsetGet( float *po );
sgVec3 *PilotOffsetGet();
void PilotOffsetSet( int opt, float setting);
float PilotOffsetGetSetting( int opt );

#endif // _VEC3_SLIDER_H
