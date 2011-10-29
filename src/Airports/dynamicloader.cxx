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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdlib>

#include "dynamicloader.hxx"

FGAirportDynamicsXMLLoader::FGAirportDynamicsXMLLoader(FGAirportDynamics* dyn):
    XMLVisitor(), _dynamics(dyn) {}

void  FGAirportDynamicsXMLLoader::startXML () {
  //cout << "FGAirportDynamicsLoader::Start XML" << endl;
}

void  FGAirportDynamicsXMLLoader::endXML () {
  //cout << "End XML" << endl;
}

void  FGAirportDynamicsXMLLoader::startElement (const char * name, const XMLAttributes &atts) {
  // const char *attval;
  FGParking park;
  FGTaxiNode taxiNode;
  FGTaxiSegment taxiSegment;
  int index = 0;
  string idxStr;
  taxiSegment.setIndex(index);
  //cout << "Start element " << name << endl;
  string attname;
  string value;
  string gateName;
  string gateNumber;
  string attval;
  string lat;
  string lon;
  int holdPointType;
  int pushBackPoint;
  
  if (name == string("Parking"))
    {
      pushBackPoint = 0;
      for (int i = 0; i < atts.size(); i++)
	{
	  //cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl; 
	  attname = atts.getName(i);
	  if (attname == string("index")) {
	        park.setIndex(std::atoi(atts.getValue(i)));
                idxStr = atts.getValue(i);
          }
	  else if (attname == string("type"))
	    park.setType(atts.getValue(i));
	 else if (attname == string("name"))
	   gateName = atts.getValue(i);
	  else if (attname == string("number"))
	    gateNumber = atts.getValue(i);
	  else if (attname == string("lat"))
	   park.setLatitude(atts.getValue(i));
	  else if (attname == string("lon"))
	    park.setLongitude(atts.getValue(i)); 
	  else if (attname == string("heading"))
	    park.setHeading(std::atof(atts.getValue(i)));
	  else if (attname == string("radius")) {
	    string radius = atts.getValue(i);
	    if (radius.find("M") != string::npos)
	      radius = radius.substr(0, radius.find("M",0));
	    //cerr << "Radius " << radius <<endl;
	    park.setRadius(std::atof(radius.c_str()));
	  }
	  else if (attname == string("airlineCodes"))
	     park.setCodes(atts.getValue(i));
          else if (attname == string("pushBackRoute")) {
             pushBackPoint = std::atoi(atts.getValue(i));
	     //park.setPushBackPoint(std::atoi(atts.getValue(i)));

           }
	}
      park.setPushBackPoint(pushBackPoint);
      park.setName((gateName+gateNumber));
      //cerr << "Parking " << idxStr << "( " << gateName << gateNumber << ") has pushBackPoint " << pushBackPoint << endl;
      _dynamics->addParking(park);
    }
  if (name == string("node")) 
    {
      for (int i = 0; i < atts.size() ; i++)
	{
	  attname = atts.getName(i);
	  if (attname == string("index"))
	    taxiNode.setIndex(std::atoi(atts.getValue(i)));
	  if (attname == string("lat"))
	    taxiNode.setLatitude(atts.getValue(i));
	  if (attname == string("lon"))
	    taxiNode.setLongitude(atts.getValue(i));
	  if (attname == string("isOnRunway"))
            taxiNode.setOnRunway((bool) std::atoi(atts.getValue(i)));
	  if (attname == string("holdPointType")) {
            attval = atts.getValue(i);
            if (attval==string("none")) {
                holdPointType=0;
            } else if (attval==string("normal")) {
                 holdPointType=1;
            } else if (attval==string("CAT II/III")) {
                 holdPointType=3;
            } else if (attval==string("PushBack")) {
                 holdPointType=3;
            } else {
                 holdPointType=0;
            }
            //cerr << "Setting Holding point to " << holdPointType << endl;
            taxiNode.setHoldPointType(holdPointType);
          }
	}
      _dynamics->getGroundNetwork()->addNode(taxiNode);
    }
  if (name == string("arc")) 
    {
      taxiSegment.setIndex(++index);
      for (int i = 0; i < atts.size() ; i++)
	{
	  attname = atts.getName(i);
	  if (attname == string("begin"))
	    taxiSegment.setStartNodeRef(std::atoi(atts.getValue(i)));
	  if (attname == string("end"))
	    taxiSegment.setEndNodeRef(std::atoi(atts.getValue(i)));
          if (attname == string("isPushBackRoute"))
	    taxiSegment.setPushBackType((bool) std::atoi(atts.getValue(i)));
	}
      _dynamics->getGroundNetwork()->addSegment(taxiSegment);
    }
  // sort by radius, in asending order, so that smaller gates are first in the list
}

void  FGAirportDynamicsXMLLoader::endElement (const char * name) {
  //cout << "End element " << name << endl;
  if (name == string("version")) {
       _dynamics->getGroundNetwork()->addVersion(atoi(value.c_str()));
       //std::cerr << "version" << value<< std::endl;
  }
  if (name == string("AWOS")) {
       _dynamics->addAwosFreq(atoi(value.c_str()));
       //cerr << "Adding AWOS" << value<< endl;
  }
  if (name == string("UNICOM")) {
       _dynamics->addUnicomFreq(atoi(value.c_str()));
       //cerr << "UNICOM" << value<< endl;
  }
if (name == string("CLEARANCE")) {
       _dynamics->addClearanceFreq(atoi(value.c_str()));
       //cerr << "Adding CLEARANCE" << value<< endl;
  }
if (name == string("GROUND")) {
       _dynamics->addGroundFreq(atoi(value.c_str()));
       //cerr << "Adding GROUND" << value<< endl;
  }

if (name == string("TOWER")) {
       _dynamics->addTowerFreq(atoi(value.c_str()));
      //cerr << "Adding TOWER" << value<< endl;
  }
if (name == string("APPROACH")) {
       _dynamics->addApproachFreq(atoi(value.c_str()));
       //cerr << "Adding approach" << value<< endl;
  }

}

void  FGAirportDynamicsXMLLoader::data (const char * s, int len) {
  string token = string(s,len);
  //cout << "Character data " << string(s,len) << endl;
  if ((token.find(" ") == string::npos && (token.find('\n')) == string::npos))
    value += token;
  else
    value = string("");
}

void  FGAirportDynamicsXMLLoader::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void  FGAirportDynamicsXMLLoader::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void  FGAirportDynamicsXMLLoader::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}
