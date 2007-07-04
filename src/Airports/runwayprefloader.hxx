#ifndef _RUNWAY_PREF_LOADER_HXX_
#define _RUNWAY_PREF_LOADER_HXX_

#include <time.h>
#include <string>

#include <simgear/compiler.h>
#include <simgear/xml/easyxml.hxx>

#include "runwayprefs.hxx"

SG_USING_STD(string);

class FGRunwayPreferenceXMLLoader : public XMLVisitor {
public:
    FGRunwayPreferenceXMLLoader(FGRunwayPreference* p);

protected:
    virtual void startXML (); 
    virtual void endXML   ();
    virtual void startElement (const char * name, const XMLAttributes &atts);
    virtual void endElement (const char * name);
    virtual void data (const char * s, int len);
    virtual void pi (const char * target, const char * data);
    virtual void warning (const char * message, int line, int column);
    virtual void error (const char * message, int line, int column);

    time_t processTime(const string &tme);

private:
    FGRunwayPreference* _pref;

    string value;

    string scheduleName;
    ScheduleTime currTimes; // Needed for parsing;

    RunwayList  rwyList;
    RunwayGroup rwyGroup;
};

#endif
