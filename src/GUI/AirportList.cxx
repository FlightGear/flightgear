#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <Main/globals.hxx>
#include <Airports/simple.hxx>

#include "AirportList.hxx"

AirportList::AirportList(int x, int y, int width, int height) :
    puaList(x, y, width, height),
    GUI_ID(FGCLASS_AIRPORTLIST),
    _content(0)
{
    create_list();
}


AirportList::~AirportList()
{
    destroy_list();
}


void
AirportList::create_list()
{
   char **content = FGAirport::searchNamesAndIdents(_filter);
   int n = (content[0] != NULL) ? 1 : 0;
    
    // work around plib 2006/04/18 bug: lists with no entries cause crash on arrow-up
    newList(n > 0 ? content : 0);

    if (_content)
        destroy_list();

    _content = content;
}


void
AirportList::destroy_list()
{
    for (char **c = _content; *c; c++) {
        delete *c;
        *c = 0;
    }
    delete [] _content;
}


void
AirportList::setValue(const char *s)
{
    std::string filter(s);
    const std::ctype<char> &ct = std::use_facet<std::ctype<char> >(std::locale());
    ct.toupper((char *)filter.data(), (char *)filter.data() + filter.size());

    if (filter != _filter) {
        _filter = filter;
        create_list();
    }
}


