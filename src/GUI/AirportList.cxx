
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
    int num_apt = _airports->size();
    char **content = new char *[num_apt + 1];

    int n = 0;
    for (int i = 0; i < num_apt; i++) {
        const FGAirport *apt = _airports->getAirport(i);
        STD::string entry(apt->getName() + "   (" + apt->getId() + ')');

        if (!_filter.empty() && entry.find(_filter) == STD::string::npos)
            continue;

        content[n] = new char[entry.size() + 1];
        strcpy(content[n], entry.c_str());
        n++;
    }
    content[n] = 0;
    // work around plib 2006/04/18 bug: lists with no entries cause crash on arrow-up
    newList(n > 0 ? content : 0);

    if (_content)
        destroy_list();

    _content = content;
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

