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

    virtual void create_list();
    virtual void destroy_list();

    // FIXME: add other string value functions
    virtual char * getListStringValue ();
    virtual void setValue (const char *);

 private:
    FGAirportList * _airports;
    char ** _content;
    STD::string _filter;
};

#endif // __AIRPORTLIST_HXX

// end of AirportList.hxx
