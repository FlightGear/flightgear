#ifndef FG_POSITIONED_BINDING_HXX
#define FG_POSITIONED_BINDING_HXX

#include <simgear/props/tiedpropertylist.hxx>

#include "positioned.hxx"

// forward decls
class FGNavRecord;
class FGRunway;
class FGAirport;

namespace flightgear
{

// forward decls
class CommStation;

class PositionedBinding
{
public:
    virtual ~PositionedBinding();
    
    static void bind(FGPositioned* pos, SGPropertyNode* node);
    

    PositionedBinding(const FGPositioned* pos, SGPropertyNode* node);

protected:
    FGPositionedRef p; // bindings own a reference to their positioned
    simgear::TiedPropertyList tied;
    
private:

};

class NavaidBinding : public PositionedBinding
{
public:
    NavaidBinding(const FGNavRecord* nav, SGPropertyNode* node);  
};

class RunwayBinding : public PositionedBinding
{
public:
    RunwayBinding(const FGRunway* rwy, SGPropertyNode* node);   
};

class AirportBinding : public PositionedBinding
{
public:
    AirportBinding(const FGAirport* apt, SGPropertyNode* node);   
};

class CommStationBinding : public PositionedBinding
{
public:
    CommStationBinding(const CommStation* sta, SGPropertyNode* node);  
};

} // of namespace flightgear

#endif // of FG_POSITIONED_BINDING_HXX
