// waypoint.hxx - waypoints that can occur in routes/procedures
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

#ifndef FG_WAYPOINT_HXX
#define FG_WAYPOINT_HXX

#include <Airports/airports_fwd.hxx>
#include <Navaids/route.hxx>
#include <Navaids/positioned.hxx>
#include <Navaids/airways.hxx>

namespace flightgear
{

class BasicWaypt : public Waypt
{
public:
  
  BasicWaypt(const SGGeod& aPos, const std::string& aIdent, RouteBase* aOwner);
    
  BasicWaypt(RouteBase* aOwner);
  
  virtual SGGeod position() const
    { return _pos; }
  
  virtual std::string ident() const
    { return _ident; }

    std::string icaoDescription() const override;
protected:
    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

    virtual std::string type() const
    { return "basic"; } 

  SGGeod _pos;
  std::string _ident;
  
};

/**
 * Waypoint based upon a navaid. In practice this means any Positioned
 * element, excluding runways (see below)
 */
class NavaidWaypoint : public Waypt
{
public:
  NavaidWaypoint(FGPositioned* aPos, RouteBase* aOwner);
  
  NavaidWaypoint(RouteBase* aOwner);
  
  virtual SGGeod position() const;
  
  virtual FGPositioned* source() const
    { return _navaid; }
    
  virtual std::string ident() const;

  protected:
  bool initFromProperties(SGPropertyNode_ptr aProp) override;
  void writeToProperties(SGPropertyNode_ptr aProp) const override;

  virtual std::string type() const
  { return "navaid"; }
    
  FGPositionedRef _navaid;
};

class OffsetNavaidWaypoint : public NavaidWaypoint
{
public:	
  OffsetNavaidWaypoint(FGPositioned* aPos, RouteBase* aOwner, double aRadial, double aDistNm);

  OffsetNavaidWaypoint(RouteBase* aOwner);
  
  virtual SGGeod position() const
    { return _geod; }

protected:
    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

    virtual std::string type() const
    { return "offset-navaid"; }
    
private:
  void init();
  
  SGGeod _geod;
  double _radial; // true, degrees
  double _distanceNm;
};

/**
 * Waypoint based upon a runway. 
 * Runways are handled specially in various places, so it's cleaner
 * to be able to distuinguish them from other navaid waypoints
 */
class RunwayWaypt : public Waypt
{
public:
  RunwayWaypt(FGRunway* aPos, RouteBase* aOwner);
  
  RunwayWaypt(RouteBase* aOwner);
  
  virtual SGGeod position() const;
  
  virtual FGPositioned* source() const;
    
  virtual std::string ident() const;

  FGRunway* runway() const
    { return _runway; }

  virtual double headingRadialDeg() const;
protected:	
  virtual std::string type() const
    { return "runway"; }

    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

private:
  FGRunway* _runway;
};

class Hold : public BasicWaypt
{
public:
  Hold(const SGGeod& aPos, const std::string& aIdent, RouteBase* aOwner);
  
  Hold(RouteBase* aOwner);
  
  void setHoldRadial(double aInboundRadial);
  void setHoldDistance(double aDistanceNm);
  void setHoldTime(double aTimeSec);
  
  void setRightHanded();
  void setLeftHanded();
  
  double inboundRadial() const
  { return _bearing; }
  
  bool isLeftHanded() const
  { return !_righthanded; }
  
  bool isDistance() const
  { return _isDistance; }
  
  double timeOrDistance() const
  { return _holdTD;}
  
  virtual double headingRadialDeg() const
  { return inboundRadial(); }
protected:
    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

    virtual std::string type() const
    { return "hold"; }
    
private:
  double _bearing;
  bool _righthanded;
  bool _isDistance;
  double _holdTD;
};

class HeadingToAltitude : public Waypt
{
public:
  HeadingToAltitude(RouteBase* aOwner, const std::string& aIdent, double aMagHdg);
  
  HeadingToAltitude(RouteBase* aOwner);

  bool initFromProperties(SGPropertyNode_ptr aProp) override;
  void writeToProperties(SGPropertyNode_ptr aProp) const override;

  virtual std::string type() const
    { return "hdgToAlt"; }

  virtual SGGeod position() const
    { return SGGeod(); }
    
  virtual std::string ident() const
    { return _ident; }
    
  double headingDegMagnetic() const
    { return _magHeading; }
  
  virtual double magvarDeg() const
    { return 0.0; }
  
  virtual double headingRadialDeg() const
  { return headingDegMagnetic(); }
private:
  std::string _ident;
  double _magHeading;
};

class DMEIntercept : public Waypt
{
public:
  DMEIntercept(RouteBase* aOwner, const std::string& aIdent, const SGGeod& aPos,
    double aCourseDeg, double aDistanceNm);
  
  DMEIntercept(RouteBase* aOwner);

  bool initFromProperties(SGPropertyNode_ptr aProp) override;
  void writeToProperties(SGPropertyNode_ptr aProp) const override;

  virtual std::string type() const
    { return "dmeIntercept"; }

  virtual SGGeod position() const
    { return _pos; }
    
  virtual std::string ident() const
    { return _ident; }
  
  double courseDegMagnetic() const
    { return _magCourse; }
    
  double dmeDistanceNm() const
    { return _dmeDistanceNm; }
  
  virtual double headingRadialDeg() const
  { return courseDegMagnetic(); }
private:
  std::string _ident;
  SGGeod _pos;
  double _magCourse;
  double _dmeDistanceNm;
};

class RadialIntercept : public Waypt
{
public:
  RadialIntercept(RouteBase* aOwner, const std::string& aIdent, const SGGeod& aPos,
    double aCourseDeg, double aRadialDeg);
  
  RadialIntercept(RouteBase* aOwner);

  bool initFromProperties(SGPropertyNode_ptr aProp) override;
  void writeToProperties(SGPropertyNode_ptr aProp) const override;

  virtual std::string type() const
    { return "radialIntercept"; }

  virtual SGGeod position() const
    { return _pos; }
    
  virtual std::string ident() const
    { return _ident; }
  
  double courseDegMagnetic() const
    { return _magCourse; }
    
  double radialDegMagnetic() const
    { return _radial; }

  virtual double headingRadialDeg() const
  { return courseDegMagnetic(); }
private:
  std::string _ident;
  SGGeod _pos;
  double _magCourse;
  double _radial;
};


/** 
 * Represent ATC radar vectored segment. Common at the end of published
 * missed approach procedures, and from STAR arrival points to final approach
 */
class ATCVectors : public Waypt
{
public:
  ATCVectors(RouteBase* aOwner, FGAirport* aFacility);
  virtual ~ATCVectors();
  
  ATCVectors(RouteBase* aOwner);

  bool initFromProperties(SGPropertyNode_ptr aProp) override;
  void writeToProperties(SGPropertyNode_ptr aProp) const override;

  virtual std::string type() const
    { return "vectors"; }

  virtual SGGeod position() const;
    
  virtual std::string ident() const;
  
private:
  /**
   * ATC facility. Using an airport here is incorrect, since often arrivals
   * facilities will be shared between several nearby airports, but it
   * suffices until we have a proper facility representation
   */
  FGAirportRef _facility;
};

/**
 * Represent a route discontinuity. These can occur while editing
 * plans via certain interfaces (such as CDUs)
 */
class Discontinuity : public Waypt
{
public:
    virtual ~Discontinuity();
    Discontinuity(RouteBase* aOwner);

    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

    virtual std::string type() const
    { return "discontinuity"; }

    virtual SGGeod position() const;

    virtual std::string ident() const;

    virtual double magvarDeg() const
    { return 0.0; }
private:
};

class Via : public Waypt
{
public:
    Via(RouteBase* aOwner);
    Via(RouteBase* aOwner, AirwayRef airway, FGPositionedRef to);
    virtual ~Via();

    bool initFromProperties(SGPropertyNode_ptr aProp) override;
    void writeToProperties(SGPropertyNode_ptr aProp) const override;

    std::string type() const override
    { return "via"; }

    SGGeod position() const override;

    std::string ident() const override;

    AirwayRef airway() const
    { return _airway; }

    FGPositioned* source() const override
    { return _to.ptr(); }

    WayptVec expandToWaypoints(WayptRef aPreceeding) const;
private:
    AirwayRef _airway;
    FGPositionedRef _to;
};
  
} // of namespace flighgear

#endif // of FG_WAYPOINT_HXX
