// FGGround - a class to provide ground control at larger airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "ground.hxx"

void FGGround::Init() {
    display = false;
}

void FGGround::Update() {
    // Each time step, what do we need to do?
    // We need to go through the list of outstanding requests and acknowedgements
    // and process at least one of them.
    // We need to go through the list of planes under our control and check if
    // any need to be addressed.
    // We need to check for planes not under our control coming within our 
    // control area and address if necessary.

    // Lets take the example of a plane which has just contacted ground
    // following landing - presumably requesting where to go?
    // First we need to establish the position of the plane within the logical network.
    // Next we need to decide where its going. 
}

void FGGround::NewArrival(plane_rec plane) {
    // What are we going to do here?
    // We need to start a new ground_rec and add the plane_rec to it
    // We need to decide what gate we are going to clear it to.
    // Then we need to add clearing it to that gate to the pending transmissions queue? - or simply transmit?
    // Probably simply transmit for now and think about a transmission queue later if we need one.
    // We might need one though in order to add a little delay for response time.
    ground_rec* g = new ground_rec;
    g->plane_rec = plane;
    g->current_pos = ConvertWGS84ToXY(plane.pos);
    g->node = GetNode(g->current_pos);  // TODO - might need to sort out node/arc here
    AssignGate(g);
    g->cleared = false;
    ground_traffic.push_back(g);
    NextClearance(g);
}

void FGGround::NewContact(plane_rec plane) {
    // This is a bit of a convienience function at the moment and is likely to change.
    if(at a gate or apron)
	NewDeparture(plane);
    else
	NewArrival(plane);
}

void FGGround::NextClearance(ground_rec &g) {
    // Need to work out where we can clear g to.
    // Assume the pilot doesn't need progressive instructions
    // We *should* already have a gate or holding point assigned by the time we get here
    // but it wouldn't do any harm to check.

    // For now though we will hardwire it to clear to the final destination.
}

void FGGround::AssignGate(ground_rec &g) {
    // We'll cheat for now - since we only have the user's aircraft and a couple of airports implemented
    // we'll hardwire the gate!
    // In the long run the logic of which gate or area to send the plane to could be somewhat non-trivial.
}