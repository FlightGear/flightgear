#include <Main/globals.hxx>
#include <Airports/simple.hxx>

#include "AirportList.hxx"


AirportList::AirportList (int x, int y, int width, int height)
    : puList(x, y, width, height)
{
    char buf[1024];

    _airports = globals->get_airports();
    _nAirports = _airports->size();

    _content = new char *[_nAirports+1];
    for (int i = 0; i < _nAirports; i++) {
        const FGAirport * airport = _airports->getAirport(i);
        snprintf(buf, 1023, "%s  %s\n",
                 airport->id.c_str(),
                 airport->name.c_str());
        _content[i] = strndup(buf, 1023);
    }
    _content[_nAirports] = 0;
    newList(_content);
}

AirportList::~AirportList ()
{
    for (int i = 0; i < _nAirports; i++) {
        delete _content[i];
        _content[i] = 0;
    }
    delete [] _content;
}

char *
AirportList::getStringValue ()
{
    return (char *)_airports->getAirport(getIntegerValue())->id.c_str();
}

// end of AirportList.cxx

