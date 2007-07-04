#ifndef _DYNAMIC_LOADER_HXX_
#define _DYNAMIC_LOADER_HXX_

#include <simgear/xml/easyxml.hxx>

#include "dynamics.hxx"

class FGAirportDynamicsXMLLoader : public XMLVisitor {
public:
    FGAirportDynamicsXMLLoader(FGAirportDynamics* dyn);

protected:
    virtual void startXML (); 
    virtual void endXML   ();
    virtual void startElement (const char * name, const XMLAttributes &atts);
    virtual void endElement (const char * name);
    virtual void data (const char * s, int len);
    virtual void pi (const char * target, const char * data);
    virtual void warning (const char * message, int line, int column);
    virtual void error (const char * message, int line, int column);

private:
    FGAirportDynamics* _dynamics;
};

#endif
