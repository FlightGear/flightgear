/**
 * route.hxx - defines basic route and route-element classes. Route elements
 * are specialised into waypoints and related things. Routes are any class tha
 * owns a collection (list, tree, graph) of route elements - such as airways,
 * procedures or a flight plan.
 */
 
// Written by James Turner, started 2009.
//
// Copyright (C) 2009  Curtis L. Olson
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef FG_ROUTE_HXX
#define FG_ROUTE_HXX

// std
#include <vector>
#include <map>
#include <iosfwd>

// Simgear
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props.hxx>

// forward decls
class FGPositioned;
class SGPath;
class FGAirport;

namespace flightgear
{

// forward decls
class Route;
class Waypt;
class NavdataVisitor;

typedef SGSharedPtr<Waypt> WayptRef;

typedef enum {
	WPT_MAP           = 1 << 0, ///< missed approach point
	WPT_IAF           = 1 << 1, ///< initial approach fix
	WPT_FAF           = 1 << 2, ///< final approach fix
	WPT_OVERFLIGHT    = 1 << 3, ///< must overfly the point directly
	WPT_TRANSITION    = 1 << 4, ///< transition to/from enroute structure
	WPT_MISS          = 1 << 5, ///< segment is part of missed approach
  /// waypoint position is dynamic, i.e moves based on other criteria,
  /// such as altitude, inbound course, or so on.
  WPT_DYNAMIC       = 1 << 6,
  /// waypoint was created automatically (not manually entered/loaded)
  /// for example waypoints from airway routing or a procedure  
  WPT_GENERATED     = 1 << 7,
  
  WPT_DEPARTURE     = 1 << 8,
  WPT_ARRIVAL       = 1 << 9
} WayptFlag;

typedef enum {
	RESTRICT_NONE,
	RESTRICT_AT,
	RESTRICT_ABOVE,
	RESTRICT_BELOW
} RouteRestriction;

/**
 * Abstract base class for waypoints (and things that are treated similarly
 * by navigation systems)
 */
class Waypt : public SGReferenced
{
public:
	Route* owner() const 
		{ return _owner; }
  
  /**
   * Return true course (in degrees) and distance (in metres) from the provided
   * position to this waypoint
   */
  virtual std::pair<double, double> courseAndDistanceFrom(const SGGeod& aPos) const;
                  	
	virtual SGGeod position() const = 0;
	
	/**
	 * The Positioned associated with this element, if one exists
	 */
	virtual FGPositioned* source() const
		{ return NULL; }
	
	virtual double altitudeFt() const 
		{ return _altitudeFt; }
		
	virtual double speedKts() const
		{ return _speedKts; }
	
	virtual RouteRestriction altitudeRestriction() const
		{ return _altRestrict; }
	
	virtual RouteRestriction speedRestriction() const
		{ return _speedRestrict; }
	
  void setAltitude(double aAlt, RouteRestriction aRestrict);
  void setSpeed(double aSpeed, RouteRestriction aRestrict);
  
  /**
   * Identifier assoicated with the waypoint. Human-readable, but
   * possibly quite terse, and definitiely not unique.
   */
	virtual std::string ident() const;
	
	/**
	 * Test if the specified flag is set for this element
	 */
	virtual bool flag(WayptFlag aFlag) const;
	
  void setFlag(WayptFlag aFlag, bool aV = true);
  
  /**
   * Factory method
   */
  static WayptRef createFromProperties(Route* aOwner, SGPropertyNode_ptr aProp);
  
  void saveAsNode(SGPropertyNode* node) const;
  
  /**
   * Test if this element and another are 'the same', i.e matching
   * ident and lat/lon are approximately equal
   */
  bool matches(Waypt* aOther) const;

  /**
   * Test if this element and a position 'the same'
   * this can be defined by either position, ident or both
   */
  bool matches(const SGGeod& aPos) const;
  
  virtual std::string type() const = 0;
protected:
  friend class NavdataVisitor;
  
	Waypt(Route* aOwner);
  
  /**
   * Persistence helper - read node properties from a file
   */
  virtual void initFromProperties(SGPropertyNode_ptr aProp);
  
  /**
   * Persistence helper - save this element to a node
   */
  virtual void writeToProperties(SGPropertyNode_ptr aProp) const;
  
  typedef Waypt* (FactoryFunction)(Route* aOwner) ;
  static void registerFactory(const std::string aNodeType, FactoryFunction* aFactory);
  
  double _altitudeFt;
	double _speedKts;
	RouteRestriction _altRestrict;
	RouteRestriction _speedRestrict;
private:

  /**
   * Create an instance of a concrete subclass, or throw an exception
   */
  static Waypt* createInstance(Route* aOwner, const std::string& aTypeName);

	Route* _owner;
	unsigned short _flags;
	
};

typedef std::vector<WayptRef> WayptVec;
  
class Route
{
public:
  /**
   *
   */
  virtual std::string ident() const = 0;
  
  static void loadAirportProcedures(const SGPath& aPath, FGAirport* aApt);
  
  static void dumpRouteToFile(const WayptVec& aRoute, const std::string& aName);
  
  static void dumpRouteToLineString(const std::string& aIdent,
    const WayptVec& aRoute, std::ostream& aStream);
private:

};

} // of namespace flightgear

#endif // of FG_ROUTE_HXX
