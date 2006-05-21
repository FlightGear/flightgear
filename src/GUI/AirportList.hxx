// AirportList.hxx - scrolling list of airports.

#ifndef __AIRPORTLIST_HXX
#define __AIRPORTLIST_HXX

#include <simgear/compiler.h>
#include STL_STRING

#include <plib/pu.h>


// ugly temproary workaround for plib's lack of user defined class ids  FIXME
#define FGCLASS_LIST        0x00000001
#define FGCLASS_AIRPORTLIST 0x00000002
class GUI_ID { public: GUI_ID(int id) : id(id) {} int id; };


#include "puList.hxx"

SG_USING_STD(string);

class FGAirportList;

class AirportList : public puList, public GUI_ID
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
