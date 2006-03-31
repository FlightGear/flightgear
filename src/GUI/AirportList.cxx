#include <string.h>	// strncpy()

#include <Main/globals.hxx>
#include <Airports/simple.hxx>

#include "AirportList.hxx"


AirportList::AirportList (int x, int y, int width, int height)
    : puList(x, y, width, height),
      _airports(globals->get_airports()),
      _content(0)
{
    create_list();
}

AirportList::~AirportList ()
{
    destroy_list();
}

void
AirportList::create_list ()
{
    if (_content)
        destroy_list();

    int num_apt = _airports->size();
    _content = new char *[num_apt + 1];

    int n = 0;
    for (int i = 0; i < num_apt; i++) {
        const FGAirport *apt = _airports->getAirport(i);
        STD::string entry(apt->getName() + "   (" + apt->getId() + ')');

        if (!_filter.empty() && entry.find(_filter) == STD::string::npos)
            continue;

        _content[n] = new char[entry.size() + 1];
        strcpy(_content[n], entry.c_str());
        n++;
    }
    _content[n] = 0;
    newList(_content);
}

void
AirportList::destroy_list ()
{
    for (char **c = _content; *c; c++) {
        delete *c;
        *c = 0;
    }
    delete [] _content;
}

char *
AirportList::getListStringValue ()
{
    int i = getListIntegerValue();
    return i < 0 ? 0 : _content[i];
}

void
AirportList::setValue (const char *s)
{
    STD::string filter(s);
    if (filter != _filter) {
        _filter = filter;
        create_list();
    }
}

// end of AirportList.cxx

