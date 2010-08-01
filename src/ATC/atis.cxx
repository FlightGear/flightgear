/******************************************************************************
 * atis.cxx
 * Written by Durk Talsma, started August 1, 2010.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <iostream>

#include "atis.hxx"

FGAtisManager::FGAtisManager() {

}

FGAtisManager::~FGAtisManager() {

}

void FGAtisManager::init() {
    SGSubsystem::init();
}

void FGAtisManager::update ( double time ) {
    cerr << "ATIS code is running at time: " << time << endl;
}
