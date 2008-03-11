// AirportList.hxx - scrolling list of airports.

#ifndef __AIRPORTLIST_HXX
#define __AIRPORTLIST_HXX

#include <simgear/compiler.h>
#include STL_STRING

#include <plib/puAux.h>
#include "dialog.hxx"



SG_USING_STD(string);

class FGAirportList;

class AirportList : public puaList, public GUI_ID
{
 public:
    AirportList (int x, int y, int width, int height);
    virtual ~AirportList ();

    virtual void create_list();
    virtual void destroy_list();
    virtual void setValue (const char *);

 private:
    FGAirportList * _airports;
    char ** _content;
    STD::string _filter;
};

#endif // __AIRPORTLIST_HXX
