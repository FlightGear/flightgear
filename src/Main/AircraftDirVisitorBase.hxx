//
// AircraftDirVisitorBase.hxx - helper to traverse a heirarchy containing
// aircraft dirs
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//

#ifndef FG_MAIN_AIRCRAFT_DIR_VISITOR_HXX
#define FG_MAIN_AIRCRAFT_DIR_VISITOR_HXX

#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>

class AircraftDirVistorBase
{
public:
    
protected:
    enum VisitResult {
        VISIT_CONTINUE = 0,
        VISIT_DONE,
        VISIT_ERROR
    };
    
    AircraftDirVistorBase() :
        _maxDepth(2)
    {
        
    }
    
    VisitResult visitAircraftPaths()
    {
        const simgear::PathList& paths(globals->get_aircraft_paths());
        simgear::PathList::const_iterator it = paths.begin();
        for (; it != paths.end(); ++it) {
            VisitResult vr = visitDir(*it, 0);
            if (vr != VISIT_CONTINUE) {
                return vr;
            }
        } // of aircraft paths iteration
        
        // if we reach this point, search the default location (always last)
        SGPath rootAircraft(globals->get_fg_root());
        rootAircraft.append("Aircraft");
        return visitDir(rootAircraft, 0);
    }
    
    VisitResult visitPath(const SGPath& path, unsigned int depth)
    {
        if (!path.exists()) {
            return VISIT_ERROR;
        }
        
        return visit(path);
    }
    
    VisitResult visitDir(const simgear::Dir& d, unsigned int depth)
    {
        if (!d.exists()) {
            SG_LOG(SG_GENERAL, SG_WARN, "visitDir: no such path:" << d.path());
            return VISIT_CONTINUE;
        }
        
        if (depth >= _maxDepth) {
            return VISIT_CONTINUE;
        }
        
        bool recurse = true;
        simgear::PathList setFiles(d.children(simgear::Dir::TYPE_FILE, "-set.xml"));
        simgear::PathList::iterator p;
        for (p = setFiles.begin(); p != setFiles.end(); ++p) {
            
            // if we found a -set.xml at this level, don't recurse any deeper
            recurse = false;
            VisitResult vr = visit(*p);
            if (vr != VISIT_CONTINUE) {
                return vr;
            }
        } // of -set.xml iteration
        
        if (!recurse) {
            return VISIT_CONTINUE;
        }
        
        simgear::PathList subdirs(d.children(simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT));
        for (p = subdirs.begin(); p != subdirs.end(); ++p) {
            VisitResult vr = visitDir(*p, depth + 1);
            if (vr != VISIT_CONTINUE) {
                return vr;
            }
        }

        return VISIT_CONTINUE;
    } // of visitDir method
    
    virtual VisitResult visit(const SGPath& path) = 0;
    
private:
    unsigned int _maxDepth;
};

#endif // of FG_MAIN_AIRCRAFT_DIR_VISITOR_HXX
