// fgjs.cxx -- assign joystick axes to flightgear properties
//
// Written by Tony Peden, started May 2001
//
// Copyright (C) 2001  Tony Peden (apeden@earthlink.net)
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

#include <simgear/compiler.h>

#include <math.h>

#include STL_IOSTREAM
#include STL_FSTREAM
#include STL_STRING

#include <jsinput.h>

SG_USING_STD(fstream);
SG_USING_STD(cout);
SG_USING_STD(endl);
SG_USING_STD(ios);
SG_USING_STD(string);

string axes_humannames[8] = { "elevator", "ailerons", "rudder", "throttle", 
                              "mixture","propller pitch", "lateral view", 
                              "longitudinal view" 
                            };

string axes_propnames[8]={ "/controls/flight/elevator","/controls/flight/aileron",
                           "/controls/flight/rudder","/controls/engines/engine/throttle",
                           "/controls/engines/engine/mixture","/controls/engines/engine/pitch", 
                           "/sim/views/axes/lat","/sim/views/axes/long" 
                         };
                      
bool half_range[8]={ false,false,false,true,true,true,false,false };


string button_humannames[6]= { "apply left brake", 
                               "apply right brake", "step flaps up", 
                               "step flaps down","apply nose-up trim",
                               "apply nose-down trim"
                             }; 

string button_propnames[6]={ "/controls/gear/brake-left",
                             "/controls/gear/brake-right",
                             "/controls/flight/flaps",
                             "/controls/flight/flaps",
                             "/controls/flight/elevator-trim",
                             "/controls/flight/elevator-trim" 
                           };                                                   
 

float button_step[6]={ 1.0, 1.0, 0.34, -0.34, 0.001, -0.001 };

string button_repeat[6]={ "false", "false", "false", "false", 
                          "true", "true" };


void waitForButton(jsSuper *jss, int wait_ms) {
      int b,lastb;
      float axes[_JS_MAX_AXES];
      b=0;
      ulMilliSecondSleep(wait_ms);
      do {
        lastb=b;
        do {
          jss->getJoystick()->read ( &b, axes ) ;
        } while( jss->nextJoystick()); 
        
        ulMilliSecondSleep(1); 
 
      }while( lastb == b ); 
      ulMilliSecondSleep(wait_ms);
}

void writeAxisProperties(fstream &fs, int control,int joystick, int axis) {
     
     char jsDesc[80];
     snprintf(jsDesc,80,"--prop:/input/joysticks/js[%d]/axis[%d]/binding",joystick,axis);
     fs << jsDesc  << "/command=property-scale" << endl; 
     fs << jsDesc  << "/property=" << axes_propnames[control] << endl; 
     
     fs << jsDesc << "/dead-band=0.02"  << endl; 
     
     if( half_range[control] == true) {
       fs << jsDesc << "/offset=-1.0" << endl; 
       fs << jsDesc << "/factor=-0.5" << endl;
     } else {
       fs << jsDesc << "/offset=0.0" << endl; 
       fs << jsDesc << "/factor=1.0" << endl;
     }
     fs << endl;
} 

void writeButtonProperties(fstream &fs, int property,int joystick, int button) {
     
     char jsDesc[80];
     snprintf(jsDesc,80,"--prop:/input/joysticks/js[%d]/button[%d]/binding",joystick,button);
     
     fs << jsDesc << "/repeatable=" << button_repeat[property] << endl;
     fs << jsDesc << "/command=property-adjust" << endl; 
     fs << jsDesc << "/property=" << button_propnames[property] << endl;
     fs << jsDesc << "/step=" << button_step[property] << endl; 
     fs << endl; 
}    

        

      
int main(void) {
  jsInit();

  jsSuper *jss=new jsSuper();
  jsInput *jsi=new jsInput(jss);
  jsi->displayValues(false);
  // int i;
  int control=0;
  
  
  cout << "Found " << jss->getNumJoysticks() << " joystick(s)" << endl;
  
  if(jss->getNumJoysticks() <= 0) { 
    cout << "Can't find any joysticks ..." << endl;
    exit(1);
  }  
  
  jss->firstJoystick();
  do {
    cout << "Joystick " << jss->getCurrentJoystickId() << " has "
         << jss->getJoystick()->getNumAxes() << " axes" << endl;
  } while( jss->nextJoystick() ); 
  
  fstream fs("fgfsrc.js",ios::out);

  
  for(control=0;control<=7;control++) {
      cout << "Move the control you wish to use for " << axes_humannames[control]
           << endl;
      fflush( stdout );
      jsi->getInput();
      
      if(jsi->getInputAxis() != -1) {
         cout << endl << "Assigned axis " << jsi->getInputAxis() 
              << " on joystick " << jsi->getInputJoystick() 
              << " to control " << axes_humannames[control]
              << endl;
      
          writeAxisProperties( fs, control, jsi->getInputJoystick(), 
                               jsi->getInputAxis() ); 
      } else {
          cout << "Skipping Axis" << endl;
      }           
      
      cout << "Press any button for next control" << endl;
      
      waitForButton(jss,500);
      cout << endl;
  }
  
  for(control=0;control<=5;control++) {
      cout << "Press the button you wish to use to " 
           << button_humannames[control]
           << endl;
      fflush( stdout );
      jsi->getInput();
      if(jsi->getInputButton() != -1) {
         
         cout << endl << "Assigned button " << jsi->getInputButton() 
              << " on joystick " << jsi->getInputJoystick() 
              << " to control " << button_humannames[control]
              << endl;
      
          writeButtonProperties( fs, control, jsi->getInputJoystick(), 
                               jsi->getInputButton() ); 
      } else {
          cout << "Skipping..." << endl;
      }           
      
      cout << "Press any button for next axis" << endl;
      
      waitForButton(jss,500);
      cout << endl;
  }
 

  delete jsi;
  delete jss;
      
  cout << "Your joystick settings are in the file fgfsrc.js" << endl;
  cout << "Check and edit as desired (especially important if you are"
         << " using a hat switch" << endl;
  cout << "as this program will, most likely, not get those right).  ";        
  
  cout << "Once you are happy, " << endl 
       << "append its contents to your .fgfsrc or system.fgfsrc" << endl;     
     
  return 1;
}      
