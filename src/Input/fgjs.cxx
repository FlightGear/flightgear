// fgjs.cxx -- assign joystick axes to flightgear properties
//
// Updated to allow xml output & added a few bits & pieces
// Laurie Bradshaw, Jun 2005
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
SG_USING_STD(cin);
SG_USING_STD(endl);
SG_USING_STD(ios);
SG_USING_STD(string);

string axes_humannames[8] = { "Aileron", "Elevator", "Rudder", "Throttle",
                              "Mixture", "Pitch", "View Direction",
                              "View Elevation"
                            };

string axes_propnames[8]={ "/controls/flight/aileron","/controls/flight/elevator",
                           "/controls/flight/rudder","/controls/engines/engine[%d]/throttle",
                           "/controls/engines/engine[%d]/mixture","/controls/engines/engine[%d]/pitch",
                           "/sim/current-view/goal-heading-offset-deg",
                           "/sim/current-view/goal-pitch-offset-deg"
                         };

string axis_posdir[8]= { "right", "down/forward", "right", "forward", "forward", "forward", "left", "upward" };


bool half_range[8]={ false,false,false,true,true,true,false,false };

bool repeatable[8]={ false,false,false,false,false,false,true,true };

bool invert[8]= { false,false,false,false,false,false,false,false };

string button_humannames[8]= { "Left Brake", "Right Brake",
                               "Flaps Up", "Flaps Down",
                               "Elevator Trim Forward", "Elevator Trim Backward",
                               "Landing Gear Up", "Landing Gear Down"
                             };

string button_propnames[8]={ "/controls/gear/brake-left",
                             "/controls/gear/brake-right",
                             "/controls/flight/flaps",
                             "/controls/flight/flaps",
                             "/controls/flight/elevator-trim",
                             "/controls/flight/elevator-trim",
                             "/controls/gear/gear-down",
                             "/controls/gear/gear-down"
                           };

bool button_modup[8]={ true,true,false,false,false,false,false,false };

bool button_boolean[8]={ false,false,false,false,false,false,true,true };

float button_step[8]={ 1.0, 1.0, -0.34, 0.34, 0.001, -0.001, 0.0, 1.0 };

string button_repeat[8]={ "false", "false", "false", "false", "true", "true", "false", "false" };


void writeAxisXML(fstream &fs, int control, int axis) {

     char axisline[16];
     snprintf(axisline,16," <axis n=\"%d\">",axis);

     fs << axisline << endl;
     fs << "  <desc>" << axes_humannames[control] << "</desc>" << endl;
     if (half_range[control]) {
       for (int i=0; i<=7; i++) {
         fs << "  <binding>" << endl;
         fs << "   <command>property-scale</command>" << endl;
         char propertyline[256];
         snprintf(propertyline,256,axes_propnames[control].c_str(),i);
         fs << "   <property>" << propertyline << "</property>" << endl;
         fs << "   <offset type=\"double\">-1.0</offset>" << endl;
         fs << "   <factor type=\"double\">-0.5</factor>" << endl;
         fs << "  </binding>" << endl;
       }
     } else if (repeatable[control]) {
       fs << "  <low>" << endl;
       fs << "   <repeatable>true</repeatable>" << endl;
       fs << "   <binding>" << endl;
       fs << "    <command>property-adjust</command>" << endl;
       fs << "    <property>" << axes_propnames[control] << "</property>" << endl;
       if (invert[control]) {
         fs << "    <step type=\"double\">1.0</step>" << endl;
       } else {
         fs << "    <step type=\"double\">-1.0</step>" << endl;
       }
       fs << "   </binding>" << endl;
       fs << "  </low>" << endl;
       fs << "  <high>" << endl;
       fs << "   <repeatable>true</repeatable>" << endl;
       fs << "   <binding>" << endl;
       fs << "    <command>property-adjust</command>" << endl;
       fs << "    <property>" << axes_propnames[control] << "</property>" << endl;
       if (invert[control]) {
         fs << "    <step type=\"double\">-1.0</step>" << endl;
       } else {
         fs << "    <step type=\"double\">1.0</step>" << endl;
       }
       fs << "   </binding>" << endl;
       fs << "  </high>" << endl;
     } else {
       fs << "  <binding>" << endl;
       fs << "   <command>property-scale</command>" << endl;
       fs << "   <property>" << axes_propnames[control] << "</property>" << endl;
       fs << "   <dead-band type=\"double\">0.02</dead-band>" << endl;
       fs << "   <offset type=\"double\">0.0</offset>" << endl;
       if (invert[control]) {
         fs << "   <factor type=\"double\">-1.0</factor>" << endl;
       } else {
         fs << "   <factor type=\"double\">1.0</factor>" << endl;
       }
       fs << "  </binding>" << endl;
     }
     fs << " </axis>" << endl << endl;
}

void writeAxisProperties(fstream &fs, int control,int joystick, int axis) {

     char jsDesc[80];
     snprintf(jsDesc,80,"--prop:/input/joysticks/js[%d]/axis[%d]",joystick,axis);
     if( half_range[control] == true) {
       for (int i=0; i<=7; i++) {
         char bindno[64];
         snprintf(bindno,64,"/binding[%d]",i);
         char propertyline[256];
         snprintf(propertyline,256,axes_propnames[control].c_str(),i);
         fs << jsDesc << bindno << "/command=property-scale" << endl;
         fs << jsDesc << bindno << "/property=" << propertyline << endl;
         fs << jsDesc << bindno << "/offset=-1.0" << endl;
         fs << jsDesc << bindno << "/factor=-0.5" << endl;
       }
     } else if (repeatable[control]) {
       fs << jsDesc << "/low/repeatable=true" << endl;
       fs << jsDesc << "/low/binding/command=property-adjust" << endl;
       fs << jsDesc << "/low/binding/property=" << axes_propnames[control] << endl;
       if (invert[control]) {
         fs << jsDesc << "/low/binding/step=1.0" << endl;
       } else {
         fs << jsDesc << "/low/binding/step=-1.0" << endl;
       }
       fs << jsDesc << "/high/repeatable=true" << endl;
       fs << jsDesc << "/high/binding/command=property-adjust" << endl;
       fs << jsDesc << "/high/binding/property=" << axes_propnames[control] << endl;
       if (invert[control]) {
         fs << jsDesc << "/high/binding/step=-1.0" << endl;
       } else {
         fs << jsDesc << "/high/binding/step=1.0" << endl;
       }
     } else {
       fs << jsDesc << "/binding/command=property-scale" << endl;
       fs << jsDesc << "/binding/property=" << axes_propnames[control] << endl;
       fs << jsDesc << "/binding/dead-band=0.02" << endl;
       fs << jsDesc << "/binding/offset=0.0" << endl;
       if (invert[control]) {
         fs << jsDesc << "/binding/factor=-1.0" << endl;
       } else {
         fs << jsDesc << "/binding/factor=1.0" << endl;
       }
     }
     fs << endl;
}

void writeButtonXML(fstream &fs, int property, int button) {

     char buttonline[32];
     snprintf(buttonline,32," <button n=\"%d\">",button);

     fs << buttonline << endl;
     if (property==-2) {
       fs << "  <desc>View Cycle</desc>" << endl;
       fs << "  <repeatable>false</repeatable>" << endl;
       fs << "  <binding>" << endl;
       fs << "   <command>view-cycle</command>" << endl;
       fs << "   <step type=\"double\">1</step>" << endl;
       fs << "  </binding>" << endl;
     } else if (property==-1) {
       fs << "  <desc>Brakes</desc>" << endl;
       fs << "  <binding>" << endl;
       fs << "   <command>property-assign</command>" << endl;
       fs << "   <property>" << button_propnames[0] << "</property>" << endl;
       fs << "   <value type=\"double\">" << button_step[0] << "</value>" << endl;
       fs << "  </binding>" << endl;
       fs << "  <binding>" << endl;
       fs << "   <command>property-assign</command>" << endl;
       fs << "   <property>" << button_propnames[1] << "</property>" << endl;
       fs << "   <value type=\"double\">" << button_step[1] << "</value>" << endl;
       fs << "  </binding>" << endl;
       fs << "  <mod-up>" << endl;
       fs << "   <binding>" << endl;
       fs << "    <command>property-assign</command>" << endl;
       fs << "    <property>" << button_propnames[0] << "</property>" << endl;
       fs << "    <value type=\"double\">0</value>" << endl;
       fs << "   </binding>" << endl;
       fs << "   <binding>" << endl;
       fs << "    <command>property-assign</command>" << endl;
       fs << "    <property>" << button_propnames[1] << "</property>" << endl;
       fs << "    <value type=\"double\">0</value>" << endl;
       fs << "   </binding>" << endl;
       fs << "  </mod-up>" << endl;
     } else {
       fs << "  <desc>" << button_humannames[property] << "</desc>" << endl;
       if (button_modup[property]) {
         fs << "  <binding>" << endl;
         fs << "   <command>property-assign</command>" << endl;
         fs << "   <property>" << button_propnames[property] << "</property>" << endl;
         fs << "   <value type=\"double\">" << button_step[property] << "</value>" << endl;
         fs << "  </binding>" << endl;
         fs << "  <mod-up>" << endl;
         fs << "   <binding>" << endl;
         fs << "    <command>property-assign</command>" << endl;
         fs << "    <property>" << button_propnames[property] << "</property>" << endl;
         fs << "    <value type=\"double\">0</value>" << endl;
         fs << "   </binding>" << endl;
         fs << "  </mod-up>" << endl;
       } else if (button_boolean[property]) {
         fs << "  <repeatable>" << button_repeat[property] << "</repeatable>" << endl;
         fs << "  <binding>" << endl;
         fs << "   <command>property-assign</command>" << endl;
         fs << "   <property>" << button_propnames[property] << "</property>" << endl;
         fs << "   <value type=\"bool\">";
         if (button_step[property]==1) {
           fs << "true";
         } else {
           fs << "false";
         }
         fs << "</value>" << endl;
         fs << "  </binding>" << endl;
       } else {
         fs << "  <repeatable>" << button_repeat[property] << "</repeatable>" << endl;
         fs << "  <binding>" << endl;
         fs << "   <command>property-adjust</command>" << endl;
         fs << "   <property>" << button_propnames[property] << "</property>" << endl;
         fs << "   <step type=\"double\">" << button_step[property] << "</step>" << endl;
         fs << "  </binding>" << endl;
       }
     }
     fs << " </button>" << endl << endl;
}

void writeButtonProperties(fstream &fs, int property,int joystick, int button) {

     char jsDesc[80];
     snprintf(jsDesc,80,"--prop:/input/joysticks/js[%d]/button[%d]",joystick,button);

     if (property==-1) {
       fs << jsDesc << "/binding[0]/command=property-assign" << endl; 
       fs << jsDesc << "/binding[0]/property=" << button_propnames[0] << endl;
       fs << jsDesc << "/binding[0]/value=" << button_step[0] << endl; 
       fs << jsDesc << "/binding[1]/command=property-assign" << endl; 
       fs << jsDesc << "/binding[1]/property=" << button_propnames[1] << endl;
       fs << jsDesc << "/binding[1]/value=" << button_step[1] << endl; 
       fs << jsDesc << "/mod-up/binding[0]/command=property-assign" << endl; 
       fs << jsDesc << "/mod-up/binding[0]/property=" << button_propnames[0] << endl;
       fs << jsDesc << "/mod-up/binding[0]/value=0" << endl; 
       fs << jsDesc << "/mod-up/binding[1]/command=property-assign" << endl; 
       fs << jsDesc << "/mod-up/binding[1]/property=" << button_propnames[1] << endl;
       fs << jsDesc << "/mod-up/binding[1]/value=0" << endl; 
       fs << endl; 
     } else if (button_modup[property]) {
       fs << jsDesc << "/binding/command=property-assign" << endl; 
       fs << jsDesc << "/binding/property=" << button_propnames[property] << endl;
       fs << jsDesc << "/binding/value=" << button_step[property] << endl; 
       fs << jsDesc << "/mod-up/binding/command=property-assign" << endl; 
       fs << jsDesc << "/mod-up/binding/property=" << button_propnames[property] << endl;
       fs << jsDesc << "/mod-up/binding/value=0" << endl; 
       fs << endl; 
     } else if (button_boolean[property]) {
       fs << jsDesc << "/repeatable=" << button_repeat[property] << endl;
       fs << jsDesc << "/binding/command=property-assign" << endl; 
       fs << jsDesc << "/binding/property=" << button_propnames[property] << endl;
       fs << jsDesc << "/binding/value=";
       if (button_step[property]==1) {
         fs << "true";
       } else {
         fs << "false";
       }
       fs << endl << endl; 
     } else {
       fs << jsDesc << "/repeatable=" << button_repeat[property] << endl;
       fs << jsDesc << "/binding/command=property-adjust" << endl; 
       fs << jsDesc << "/binding/property=" << button_propnames[property] << endl;
       fs << jsDesc << "/binding/step=" << button_step[property] << endl; 
       fs << endl; 
     }
}

int main( int argc, char *argv[] ) {

  bool usexml=true;
  float deadband=0;
  char answer[128];
  int btninit=-2;

  for (int i=1; i<argc; i++) {
    if (strcmp("--help",argv[i])==0) {
      cout << "Usage:" << endl;
      cout << "  --help\t\t\tShow this help" << endl;
      cout << "  --prop\t\t\tOutput property list" << endl;
      cout << "  --xml\t\t\t\tOutput xml (default)" << endl;
      cout << "  --deadband <float>\t\tSet deadband (for this program only, useful" << endl;
      cout << "\t\t\t\tfor 'twitchy' joysticks)" << endl;
      exit(0);
    } else if (strcmp("--prop",argv[i])==0) {
      usexml=false;
      btninit=-1;
    } else if (strcmp("--deadband",argv[i])==0) {
      i++;
      deadband=atoi(argv[i]);
      cout << "Deadband set to " << argv[i] << endl;
    } else if (strcmp("--xml",argv[i])!=0) {
      cout << "Unknown option \"" << argv[i] << "\"" << endl;
      exit(0);
    }
  }

  jsInit();

  jsSuper *jss=new jsSuper();
  jsInput *jsi=new jsInput(jss);
  jsi->displayValues(false);
  int control=0;

  cout << "Found " << jss->getNumJoysticks() << " joystick(s)" << endl;

  if(jss->getNumJoysticks() <= 0) { 
    cout << "Can't find any joysticks ..." << endl;
    exit(1);
  }

  fstream fs;
  fstream *xfs = new fstream[jss->getNumJoysticks()];
  if (!usexml) {
    fs.open("fgfsrc.js",ios::out);
  }

  jss->firstJoystick();
  do {
    cout << "Joystick #" << jss->getCurrentJoystickId()
         << " \"" << jss->getJoystick()->getName() << "\" has "
         << jss->getJoystick()->getNumAxes() << " axes" << endl;
    for (int i=0; i<jss->getJoystick()->getNumAxes(); i++) {
      jss->getJoystick()->setDeadBand(i,deadband);
    }
    if (usexml) {
      char filename[16];
      snprintf(filename,16,"js%i.xml",jss->getCurrentJoystickId());
      xfs[jss->getCurrentJoystickId()].open(filename,ios::out);
      xfs[jss->getCurrentJoystickId()] << "<?xml version=\"1.0\" ?>" << endl
      << endl << "<PropertyList>" << endl << endl << " <name>"
      << jss->getJoystick()->getName() << "</name>" << endl << endl;
    }
  } while( jss->nextJoystick() ); 

  for(control=0;control<=7;control++) {
      cout << "Move the control you wish to use for " << axes_humannames[control]
           << " " << axis_posdir[control] << endl;
      cout << "Pressing a button skips this axis\n";
      fflush( stdout );
      jsi->getInput();

      if(jsi->getInputAxis() != -1) {
         cout << endl << "Assigned axis " << jsi->getInputAxis()
              << " on joystick " << jsi->getInputJoystick()
              << " to control " << axes_humannames[control]
              << endl;
         bool badanswer=true;
         do {
           cout << "Is this correct? (y/n) $ ";
           cin >> answer;
           if ((strcmp(answer,"y")==0) || (strcmp(answer,"n")==0)) { badanswer=false; }
         } while (badanswer);
         if (strcmp(answer,"n")==0) {
           control--;
         } else {
	   invert[control]=!jsi->getInputAxisPositive();
           if (usexml) {
             writeAxisXML( xfs[jsi->getInputJoystick()], control, jsi->getInputAxis() );
           } else {
             writeAxisProperties( fs, control, jsi->getInputJoystick(), jsi->getInputAxis() );
           }
         }
      } else {
         cout << "Skipping control" << endl;
         bool badanswer=true;
         do {
           cout << "Is this correct? (y/n) $ ";
           cin >> answer;
           if ((strcmp(answer,"y")==0) || (strcmp(answer,"n")==0)) { badanswer=false; }
         } while (badanswer);
         if (strcmp(answer,"n")==0) { control--; }
      }
      cout << endl;
  }

  for(control=btninit;control<=7;control++) {
      if ( control == -2 ) {
        cout << "Press the button you wish to use for View Cycle" << endl;
      } else if ( control == -1 ) {
        cout << "Press the button you wish to use for Brakes" << endl;
      } else {
        cout << "Press the button you wish to use for " << button_humannames[control] << endl;
      }
      cout << "Moving a joystick axis skips this button\n";
      fflush( stdout );
      jsi->getInput();
      if(jsi->getInputButton() != -1) {
         cout << endl << "Assigned button " << jsi->getInputButton()
              << " on joystick " << jsi->getInputJoystick()
              << " to control ";
         if ( control == -2 ) { cout << "View Cycle" << endl; }
         else if ( control == -1 ) { cout << "Brakes" << endl; }
         else { cout << button_humannames[control] << endl; }
         bool badanswer=true;
         do {
           cout << "Is this correct? (y/n) $ ";
           cin >> answer;
           if ((strcmp(answer,"y")==0) || (strcmp(answer,"n")==0)) { badanswer=false; }
         } while (badanswer);
         if (strcmp(answer,"n")==0) {
           control--;
         } else {
           if (usexml) {
             writeButtonXML( xfs[jsi->getInputJoystick()], control, jsi->getInputButton() );
           } else {
             writeButtonProperties( fs, control, jsi->getInputJoystick(), jsi->getInputButton() );
           }
         }
      } else {
         cout << "Skipping control" << endl;
         bool badanswer=true;
         do {
           cout << "Is this correct? (y/n) $ ";
           cin >> answer;
           if ((strcmp(answer,"y")==0) || (strcmp(answer,"n")==0)) { badanswer=false; }
         } while (badanswer);
         if (strcmp(answer,"n")==0) { control--; }
      }

      cout << endl;
  }
  if (usexml) {
    for (int i=0; i<jss->getNumJoysticks(); i++) {
      xfs[i] << "</PropertyList>" << endl << endl << "<!-- end of joystick.xml -->" << endl;
      xfs[i].close();
    }
  } else {
    fs.close();
  }
  delete jsi;
  delete[] xfs;
  delete jss;

  cout << "Your joystick settings are in ";
  if (usexml) {
    cout << "js0.xml, js1.xml, etc. depending on how many" << endl << "devices you have." << endl;
  } else {
    cout << "fgfsrc.js" << endl;
  }
  cout << endl << "Check and edit as desired. Once you are happy," << endl;
  if (usexml) {
    cout << "move relevant js<n>.xml files to $FG_ROOT/Input/Joysticks/ (if you didn't use" << endl
         << "an attached controller, you don't need to move the corresponding file)" << endl;
  } else {
    cout << "append its contents to your .fgfsrc or system.fgfsrc" << endl;
  }

  return 1;
}
