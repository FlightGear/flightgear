#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "PositionedBinding.hxx"

#include <map>

#include <simgear/props/props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_props.hxx>
#include <Navaids/navrecord.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <ATC/CommStation.hxx>

typedef std::map<SGPropertyNode*, flightgear::PositionedBinding*> BindingMap;
static BindingMap static_bindings;
    
namespace {
    
class PropertyDeleteObserver : public SGPropertyChangeListener
{
public:
    virtual void childRemoved(SGPropertyNode*, SGPropertyNode* node)
    {
        BindingMap::iterator it = static_bindings.find(node);
        if (it != static_bindings.end()) {
            SG_LOG(SG_GENERAL, SG_INFO, "saw remove of:" << node->getPath() << ", deleting binding");
            delete it->second;
            static_bindings.erase(it);
        }
    }
};
    
static PropertyDeleteObserver* static_deleteObserver = NULL;
    
} // of anonymous namespace
    
namespace flightgear
{

PositionedBinding::PositionedBinding(const FGPositioned* pos, SGPropertyNode* nd) :
    p(const_cast<FGPositioned*>(pos)),
    tied(nd)
{
    if (!static_deleteObserver) {
        static_deleteObserver = new PropertyDeleteObserver;
        globals->get_props()->addChangeListener(static_deleteObserver, false);
    }
    
    nd->setDoubleValue("latitude-deg", p->latitude());
    nd->setDoubleValue("longitude-deg", p->longitude());
    
    if (p->elevation() > -1000) {
        nd->setDoubleValue("elevation-ft", p->elevation());
    }
    
    nd->setStringValue("ident", p->ident());
    if (!p->name().empty()) {
        nd->setStringValue("name", p->name());
    }
    
    nd->setStringValue("type", FGPositioned::nameForType(p->type()));
}

PositionedBinding::~PositionedBinding()
{
    tied.Untie();
}

void PositionedBinding::bind(FGPositioned* pos, SGPropertyNode* node)
{
    BindingMap::iterator it = static_bindings.find(node);
    if (it != static_bindings.end()) {
        throw sg_exception("duplicate positioned binding", node->getPath());
    }
    
    PositionedBinding* binding = pos->createBinding(node);
    static_bindings.insert(it, std::make_pair(node, binding));
}

NavaidBinding::NavaidBinding(const FGNavRecord* nav, SGPropertyNode* nd) :
    PositionedBinding(nav, nd)
{
    FGPositioned::Type ty = nav->type();
    if (ty == FGPositioned::NDB) {
        nd->setDoubleValue("frequency-khz", nav->get_freq() / 100.0);
    } else {
        nd->setDoubleValue("frequency-mhz", nav->get_freq() / 100.0);
    }
    
    if ((ty == FGPositioned::LOC) || (ty == FGPositioned::ILS)) {
      nd->setDoubleValue("loc-course-deg", nav->get_multiuse());
    }
    
    if (ty == FGPositioned::GS) {
        nd->setDoubleValue("gs-angle-deg", nav->get_multiuse());
    }
    
    nd->setDoubleValue("range-nm", nav->get_range());
    
    if (nav->runway()) {
        // don't want to create a cycle in the graph, so we don't re-bind
        // the airport/runway node here - just expose the IDs
        nd->setStringValue("airport", nav->runway()->airport()->ident());
        nd->setStringValue("runway", nav->runway()->ident());
    }
};

RunwayBinding::RunwayBinding(const FGRunway* rwy, SGPropertyNode* nd) :
    PositionedBinding(rwy, nd)
{
    nd->setDoubleValue("length-ft", rwy->lengthFt());
    nd->setDoubleValue("length-m", rwy->lengthM());
    nd->setDoubleValue("width-ft", rwy->widthFt());
    nd->setDoubleValue("width-m", rwy->widthM());
    nd->setDoubleValue("heading-deg", rwy->headingDeg());
    nd->setBoolValue("hard-surface", rwy->isHardSurface());
    
    nd->setDoubleValue("threshold-displacement-m", rwy->displacedThresholdM());
    nd->setDoubleValue("stopway-m", rwy->stopwayM());
    
    if (rwy->ILS()) {
        SGPropertyNode* ilsNode = nd->getChild("ils", 0, true);
        PositionedBinding::bind(rwy->ILS(), ilsNode);
    }
}

AirportBinding::AirportBinding(const FGAirport* apt, SGPropertyNode* nd) :
    PositionedBinding(apt, nd)
{
    nd->setIntValue("num-runways", apt->numRunways());
    
    SGGeod tower = apt->getTowerLocation();
    nd->setDoubleValue("tower/latitude-deg", tower.getLatitudeDeg());
    nd->setDoubleValue("tower/longitude-deg", tower.getLongitudeDeg());
    nd->setDoubleValue("tower/elevation-ft", tower.getElevationFt());
    
    for (unsigned int r=0; r<apt->numRunways(); ++r) {
        SGPropertyNode* rn = nd->getChild("runway", r, true);
        FGRunway* rwy = apt->getRunwayByIndex(r);
        PositionedBinding::bind(rwy, rn);
    }
    
    for (unsigned int c=0; c<apt->commStations().size(); ++c) {
        flightgear::CommStation* comm = apt->commStations()[c];
        std::string tynm = FGPositioned::nameForType(comm->type());
        
      // for some standard frequence types, we don't care about the ident,
      // so just list the frequencies under one group.
        if ((comm->type() == FGPositioned::FREQ_ATIS) ||
            (comm->type() == FGPositioned::FREQ_AWOS) ||
            (comm->type() == FGPositioned::FREQ_TOWER) ||
            (comm->type() == FGPositioned::FREQ_GROUND))
        {
          SGPropertyNode* commNode = nd->getChild(tynm, 0, true);
          int count = nd->getChildren("frequency-mhz").size();
          SGPropertyNode* freqNode = commNode->getChild("frequency-mhz", count, true);
          freqNode->setDoubleValue(comm->freqMHz());
        } else {
      // for other kinds of frequency, there's more variation, so list the ID too
          int count = nd->getChildren(tynm).size();
          SGPropertyNode* commNode = nd->getChild(tynm, count, true);
          commNode->setStringValue("ident", comm->ident());
          commNode->setDoubleValue("frequency-mhz", comm->freqMHz());

        }
      } // of airprot comm stations iteration
}

CommStationBinding::CommStationBinding(const CommStation* sta, SGPropertyNode* node) :
    PositionedBinding(sta, node)
{
    node->setIntValue("range-nm", sta->rangeNm());
    node->setDoubleValue("frequency-mhz", sta->freqMHz());
    
    if (sta->airport()) {
        // don't want to create a cycle in the graph, so we don't re-bind
        // the airport/runway node here - just expose the IDs
        node->setStringValue("airport", sta->airport()->ident());
    }
}

} // of namespace flightgear

