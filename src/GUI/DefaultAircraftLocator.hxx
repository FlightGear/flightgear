#ifndef DEFAULTAIRCRAFTLOCATOR_HXX
#define DEFAULTAIRCRAFTLOCATOR_HXX

#include <string>
#include <simgear/misc/sg_path.hxx>

#include <Main/AircraftDirVisitorBase.hxx>

namespace flightgear
{

std::string defaultAirportICAO();

/**
 * we don't want to rely on the main AircraftModel threaded scan, to find the
 * default aircraft, so we do a synchronous scan here, on the assumption that
 * FG_DATA/Aircraft only contains a handful of entries.
 */
class DefaultAircraftLocator : public AircraftDirVistorBase
{
public:
    DefaultAircraftLocator();

    SGPath foundPath() const;

private:
    virtual VisitResult visit(const SGPath& p) override;

    std::string _aircraftId;
    SGPath _foundPath;
};

}

#endif // DEFAULTAIRCRAFTLOCATOR_HXX
