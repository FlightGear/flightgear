#include <simgear/misc/sg_path.hxx>
#include <Main/globals.hxx>

#include "xmlloader.hxx"
#include "dynamicloader.hxx"
#include "runwayprefloader.hxx"

#include "dynamics.hxx"
#include "runwayprefs.hxx"

XMLLoader::XMLLoader() {}
XMLLoader::~XMLLoader() {}

void XMLLoader::load(FGAirportDynamics* d) {
  FGAirportDynamicsXMLLoader visitor(d);

  SGPath parkpath( globals->get_fg_root() );
  parkpath.append( "/AI/Airports/" );
  parkpath.append( d->getId() );
  parkpath.append( "parking.xml" );
  
  if (parkpath.exists()) {
    try {
      readXML(parkpath.str(), visitor);
      d->init();
    } catch (const sg_exception &e) {
      //cerr << "unable to read " << parkpath.str() << endl;
    }
  }

}

void XMLLoader::load(FGRunwayPreference* p) {
  FGRunwayPreferenceXMLLoader visitor(p);

  SGPath rwyPrefPath( globals->get_fg_root() );
  rwyPrefPath.append( "AI/Airports/" );
  rwyPrefPath.append( p->getId() );
  rwyPrefPath.append( "rwyuse.xml" );

  //if (ai_dirs.find(id.c_str()) != ai_dirs.end()
  //  && rwyPrefPath.exists())
  if (rwyPrefPath.exists()) {
    try {
      readXML(rwyPrefPath.str(), visitor);
    } catch (const sg_exception &e) {
      //cerr << "unable to read " << rwyPrefPath.str() << endl;
    }
  }
}
