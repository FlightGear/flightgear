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
//

#include <cstring>
#include <cstdlib>

#include <simgear/debug/logstream.hxx>

#include "runwayprefloader.hxx"

using namespace std;

FGRunwayPreferenceXMLLoader::FGRunwayPreferenceXMLLoader(FGRunwayPreference* p):XMLVisitor(), _pref(p) {}

void  FGRunwayPreferenceXMLLoader::startXML () {
  //  cout << "Start XML" << endl;
}

void  FGRunwayPreferenceXMLLoader::endXML () {
  //cout << "End XML" << endl;
}

void  FGRunwayPreferenceXMLLoader::startElement (const char * name, const XMLAttributes &atts) {
  //cout << "StartElement " << name << endl;
  value = string("");
  if (!(strcmp(name, "wind"))) {
    //cerr << "Will be processing Wind" << endl;
    for (int i = 0; i < atts.size(); i++)
      {
  	//cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
  	//attname = atts.getName(i);
  	if (atts.getName(i) == string("tail")) {
  	  //cerr << "Tail Wind = " << atts.getValue(i) << endl;
  	  currTimes.setTailWind(atof(atts.getValue(i)));
  	}	
  	if (atts.getName(i) == string("cross")) {
  	  //cerr << "Cross Wind = " << atts.getValue(i) << endl;
  	  currTimes.setCrossWind(atof(atts.getValue(i)));
  	}
     }
  }
    if (!(strcmp(name, "time"))) {
      //cerr << "Will be processing time" << endl;	
    for (int i = 0; i < atts.size(); i++)
      {
  	if (atts.getName(i) == string("start")) {
  	  //cerr << "Start Time = " << atts.getValue(i) << endl;
  	  currTimes.addStartTime(processTime(atts.getValue(i)));
  	}
  	if (atts.getName(i) == string("end")) {
  	  //cerr << "End time = " << atts.getValue(i) << endl;
  	  currTimes.addEndTime(processTime(atts.getValue(i)));
  	}
  	if (atts.getName(i) == string("schedule")) {
  	  //cerr << "Schedule Name  = " << atts.getValue(i) << endl;
  	  currTimes.addScheduleName(atts.getValue(i));
  	}	
    }
  }
  if (!(strcmp(name, "takeoff"))) {
    rwyList.clear();
  }
  if  (!(strcmp(name, "landing")))
    {
      rwyList.clear();
    }
  if (!(strcmp(name, "schedule"))) {
    for (int i = 0; i < atts.size(); i++)
      {
  	//cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
  	//attname = atts.getName(i);
  	if (atts.getName(i) == string("name")) {
  	  //cerr << "Schedule name = " << atts.getValue(i) << endl;
  	  scheduleName = atts.getValue(i);
  	}
      }
  }
}

//based on a string containing hour and minute, return nr seconds since day start.
time_t FGRunwayPreferenceXMLLoader::processTime(const string &tme)
{
  string hour   = tme.substr(0, tme.find(":",0));
  string minute = tme.substr(tme.find(":",0)+1, tme.length());

  //cerr << "hour = " << hour << " Minute = " << minute << endl;
  return (atoi(hour.c_str()) * 3600 + atoi(minute.c_str()) * 60);
}

void  FGRunwayPreferenceXMLLoader::endElement (const char * name) {
  //cout << "End element " << name << endl;
  if (!(strcmp(name, "rwyuse"))) {
    _pref->setInitialized(true);
  }
  if (!(strcmp(name, "com"))) { // Commercial Traffic
    //cerr << "Setting time table for commerical traffic" << endl;
    _pref->setComTimes(currTimes);
    currTimes.clear();
  }
  if (!(strcmp(name, "gen"))) { // General Aviation
    //cerr << "Setting time table for general aviation" << endl;
    _pref->setGenTimes(currTimes);
    currTimes.clear();
  }  
  if (!(strcmp(name, "mil"))) { // Military Traffic
    //cerr << "Setting time table for military traffic" << endl;
    _pref->setMilTimes(currTimes);
    currTimes.clear();
  }
  if (!(strcmp(name, "ul"))) { // Military Traffic
    //cerr << "Setting time table for military traffic" << endl;
    _pref->setULTimes(currTimes);
    currTimes.clear();
  }


  if (!(strcmp(name, "takeoff"))) {
    //cerr << "Adding takeoff: " << value << endl;
    rwyList.set(name, value);
    rwyGroup.add(rwyList);
  }
  if (!(strcmp(name, "landing"))) {
    //cerr << "Adding landing: " << value << endl;
    rwyList.set(name, value);
    rwyGroup.add(rwyList);
  }
  if (!(strcmp(name, "schedule"))) {
    //cerr << "Adding schedule" << scheduleName << endl;
    rwyGroup.setName(scheduleName);
    //rwyGroup.addRunways(rwyList);
    _pref->addRunwayGroup(rwyGroup);
    rwyGroup.clear();
    //exit(1);
  }
}

void  FGRunwayPreferenceXMLLoader::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  //if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
  //  value += token;
  //else
  //  value = string("");
  value += token;
}

void  FGRunwayPreferenceXMLLoader::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGRunwayPreferenceXMLLoader::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void  FGRunwayPreferenceXMLLoader::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}
