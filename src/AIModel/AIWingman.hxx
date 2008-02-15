// FGAIWingman - FGAIBllistic-derived class creates an AI Wingman
//
// Written by Vivian Meazza, started February 2008.
// - vivian.meazza at lineone.net
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

#ifndef _FG_AIWINGMAN_HXX
#define _FG_AIWINGMAN_HXX

#include "AIBallistic.hxx"

#include "AIManager.hxx"
#include "AIBase.hxx"

class FGAIWingman : public FGAIBallistic {
public:
    FGAIWingman();
    virtual ~FGAIWingman();

    virtual void readFromScenario(SGPropertyNode* scFileNode);
    virtual void bind();
    virtual void unbind();
    virtual const char* getTypeString(void) const { return "wingman"; }

    bool init(bool search_in_AI_path=false);

private:

    virtual void reinit() { init(); }
    virtual void update (double dt);

};

#endif  // FG_AIWINGMAN_HXX
