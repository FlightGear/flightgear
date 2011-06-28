// FGAIStatic - FGAIBase-derived class creates an AI static object
//
// Written by David Culp, started Jun 2005.
//
// Copyright (C) 2005  David P. Culp - davidculp2@comcast.net
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>

using std::string;

#include "AIStatic.hxx"


FGAIStatic::FGAIStatic() : FGAIBase(otStatic, false) {
}


FGAIStatic::~FGAIStatic() {
}

bool FGAIStatic::init(bool search_in_AI_path) {
   return FGAIBase::init(search_in_AI_path);
}

void FGAIStatic::bind() {
    FGAIBase::bind();
}

void FGAIStatic::unbind() {
    FGAIBase::unbind();
}


void FGAIStatic::update(double dt) {
   FGAIBase::update(dt);
   Transform();
}

