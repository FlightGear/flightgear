#include <locale>
#include <Main/globals.hxx>
#include <Airports/simple.hxx>

#include "AirportList.hxx"


AirportList::AirportList(int x, int y, int width, int height) :
    puaList(x, y, width, height),
    GUI_ID(FGCLASS_AIRPORTLIST),
    _airports(globals->get_airports()),
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
    const std::ctype<char> &ct = std::use_facet<std::ctype<char> >(std::locale());
    int num_apt = _airports->size();
    char **content = new char *[num_apt + 1];

    int n = 0;
    for (int i = 0; i < num_apt; i++) {
        const FGAirport *apt = _airports->getAirport(i);
        std::string entry(' ' + apt->getName() + "   (" + apt->getId() + ')');

        if (!_filter.empty()) {
            std::string upper(entry.data());
            ct.toupper((char *)upper.data(), (char *)upper.data() + upper.size());

            if (upper.find(_filter) == std::string::npos)
                continue;
        }

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


