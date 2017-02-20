#include "DefaultAircraftLocator.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>

static SGPropertyNode_ptr loadXMLDefaults()
{
    SGPropertyNode_ptr root(new SGPropertyNode);
    const SGPath defaultsXML = globals->get_fg_root() / "defaults.xml";
    readProperties(defaultsXML, root);

    if (!root->hasChild("sim")) {
        SG_LOG(SG_GUI, SG_POPUP, "Failed to find /sim node in defaults.xml, broken");
        return SGPropertyNode_ptr();
    }

    return root;
}


namespace flightgear
{

std::string defaultAirportICAO()
{
    SGPropertyNode_ptr root = loadXMLDefaults();
    if (!root) {
        return std::string();
    }

    std::string airportCode = root->getStringValue("/sim/presets/airport-id");
    return airportCode;
}

DefaultAircraftLocator::DefaultAircraftLocator()
{
    SGPropertyNode_ptr root = loadXMLDefaults();
    if (root) {
        _aircraftId = root->getStringValue("/sim/aircraft");
    } else {
        SG_LOG(SG_GUI, SG_WARN, "failed to load default aircraft identifier");
        _aircraftId = "ufo"; // last ditch fallback
    }

    _aircraftId += "-set.xml";
    const SGPath rootAircraft = globals->get_fg_root() / "Aircraft";
    visitDir(rootAircraft, 0);
}

SGPath DefaultAircraftLocator::foundPath() const
{
    return _foundPath;
}

AircraftDirVistorBase::VisitResult
DefaultAircraftLocator::visit(const SGPath& p)
{
    if (p.file() == _aircraftId) {
        _foundPath = p;
        return VISIT_DONE;
    }

    return VISIT_CONTINUE;
}

} // of namespace flightgear
