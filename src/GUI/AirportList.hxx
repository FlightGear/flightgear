// AirportList.hxx - scrolling list of airports.

#ifndef __AIRPORTLIST_HXX

#include <plib/pu.h>

#include "puList.hxx"

class FGAirportList;

class AirportList : public puList
{
 public:
    AirportList (int x, int y, int width, int height);
    virtual ~AirportList ();

    // FIXME: add other string value functions
    virtual char * getListStringValue ();

 private:
    FGAirportList * _airports;
    int _nAirports;
    char ** _content;
};

#endif // __AIRPORTLIST_HXX

// end of AirportList.hxx
