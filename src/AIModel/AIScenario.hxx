// FGAIScenario - class for loading an AI scenario
// Written by David Culp, started May 2004
// - davidculp2@comcast.net
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

#ifndef _FG_AISCENARIO_HXX
#define _FG_AISCENARIO_HXX

#include <simgear/compiler.h>

#include <vector>
#include <string>

#include "AIBase.hxx"

SG_USING_STD(vector);
SG_USING_STD(string);


class FGAIScenario {

public:

   FGAIScenario(string filename);
   ~FGAIScenario();

   FGAIModelEntity* getNextEntry( void );
   int nEntries( void );

private:

    typedef vector <FGAIModelEntity*> entry_vector_type;
    typedef entry_vector_type::iterator entry_vector_iterator;

    entry_vector_type       entries;
    entry_vector_iterator   entry_iterator;


};    


#endif  // _FG_AISCENARIO_HXX

