/**********************************************************************

 FILENAME:     uiuc_engine.cpp

----------------------------------------------------------------------

 DESCRIPTION:  determine the engine forces and moments
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on portions of c172_engine.c, called from ls_model

----------------------------------------------------------------------

 HISTORY:      01/30/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>
               Jeff Scott         <jscott@mail.com>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -engine model

----------------------------------------------------------------------

 OUTPUTS:      -F_X_engine
               -F_Z_engine
               -M_m_engine

----------------------------------------------------------------------

 CALLED BY:   uiuc_wrapper.cpp 

----------------------------------------------------------------------

 CALLS TO:     none

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2000 by Michael Selig

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/

#include "uiuc_engine.h"

void uiuc_engine() 
{
  stack command_list;
  string linetoken1;
  string linetoken2;
  
  /* [] mss questions: why do this here ... and then undo it later? 
     ... does Throttle[3] get modified? */
  Throttle[3] = Throttle_pct;

  command_list = engineParts -> getCommands();
  
  if (command_list.begin() == command_list.end())
  {
        cerr << "ERROR: Engine not specified. Aircraft cannot fly without the engine" << endl;
        exit(-1);
  }
 
  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      //cout << *command_line << endl;
      
      linetoken1 = engineParts -> getToken(*command_line, 1); // function parameters gettoken(string,tokenNo);
      linetoken2 = engineParts -> getToken(*command_line, 2); // 2 represents token No 2
      
      switch(engine_map[linetoken2])
            {
          case simpleSingle_flag:
            {
              //c172 engine lines ... looks like 0.83 is just a thrust reduction
              /* F_X_engine = Throttle[3]*350/0.83; */
              /* F_Z_engine = Throttle[3]*4.9/0.83; */
              /* M_m_engine = F_X_engine*0.734*cbar; */
              F_X_engine = Throttle[3]*simpleSingleMaxThrust;
              break;
            }
            case c172_flag:
              {
                F_X_engine = Throttle[3]*350/0.83;
                F_Z_engine = Throttle[3]*4.9/0.83;
                M_m_engine = F_X_engine*0.734*cbar;
                break;
              }
            };
      
      Throttle_pct = Throttle[3];
      return;
    }
}

// end uiuc_engine.cpp
