/**
 * Polyline - store geographic line-segments */

// Written by James Turner, started 2013.
//
// Copyright (C) 2013 James Turner <zakalawe@mac.com>
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

#ifndef FG_POLY_LINE_HXX
#define FG_POLY_LINE_HXX

#include <vector>

#include <simgear/sg_inlines.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGBox.hxx>
#include <simgear/math/SGGeometryFwd.hxx>

namespace flightgear
{

typedef std::vector<SGGeod> SGGeodVec;
    
class PolyLine;
    
typedef SGSharedPtr<PolyLine> PolyLineRef;
typedef std::vector<PolyLineRef> PolyLineList;
    
/**
 * @class Store geographical linear data, with a type code.
 *
 * This is a basic in-memory model of GIS line data, without support for
 * many features; especially there is no support for per-node attributes.
 *
 * PolyLines are added to the spatial index and can be queried by passing
 * a search centre and cutoff distance.
 */
class PolyLine : public SGReferenced
{
public:
    virtual ~PolyLine();
    
    enum Type
    {
        INVALID = 0,
        COASTLINE,
        NATIONAL_BOUNDARY, /// aka a border
        REGIONAL_BOUNDARY, /// state / province / country / department
        RIVER,
        LAKE,
        URBAN,
        // airspace types in the future
        LAST_TYPE
    };
    
    Type type() const
    { return m_type; }
    
    /**
     * number of points in this line - at least two.
     */
    unsigned int numPoints() const;
    
    SGGeod point(unsigned int aIndex) const;
    
    const SGGeodVec& points() const
    { return m_data; }
    
    /**
     * create poly line objects from raw input points and a type.
     * input points will be subdivided so the bounding area of each
     * polyline stays within some threshold.
     *
     */
    static PolyLineList createChunked(Type aTy, const SGGeodVec& aRawPoints);
    
    static PolyLineRef create(Type aTy, const SGGeodVec& aRawPoints);

    static void bulkAddToSpatialIndex(PolyLineList::const_iterator begin,
                                      PolyLineList::const_iterator end);

    /**
     * retrieve all the lines within a range of a search point.
     * lines are returned if any point is near the search location.
     */
    static PolyLineList linesNearPos(const SGGeod& aPos, double aRangeNm, Type aTy);
    
    class TypeFilter
    {
    public:
        virtual bool pass(Type aTy) const = 0;
    };
    
    static PolyLineList linesNearPos(const SGGeod& aPos, double aRangeNm, const TypeFilter& aFilter);

    SGBoxd cartesianBox() const;

    void addToSpatialIndex() const;

private:
    
    PolyLine(Type aTy, const SGGeodVec& aPoints);
    
    Type m_type;
    SGGeodVec m_data;

};
    

    
} // of namespace flightgear

#endif
