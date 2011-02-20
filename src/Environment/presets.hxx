// presets.hxx -- Wrap environment presets
//
// Written by Torsten Dreyer, January 2011
//
// Copyright (C) 2010  Torsten Dreyer Torsten(at)t3r(dot)de
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

#ifndef __ENVIRONMENT_PRESETS_HXX
#define __ENVIRONMENT_PRESETS_HXX

#include <simgear/structure/Singleton.hxx>
#include <simgear/props/props.hxx>

namespace Environment {

/**
 * @brief A wrapper for presets of environment properties
 * mainly set from the command line with --wind=270@10, 
 * visibility=1600 etc.
 */
namespace Presets {

class PresetBase {
public:
    PresetBase( const char * overrideNodePath );
    virtual void disablePreset() { setOverride(false); }
protected:
    void setOverride( bool value );
private:
    std::string _overrideNodePath;
    SGPropertyNode_ptr _overrideNode;
};

class Ceiling : public PresetBase {
public:
    Ceiling();
    void preset( double elevation, double thickness );
private:
    SGPropertyNode_ptr _elevationNode;
    SGPropertyNode_ptr _thicknessNode;
};

typedef simgear::Singleton<Ceiling> CeilingSingleton;

class Turbulence : public PresetBase {
public:
    Turbulence();
    void preset( double magnitude_norm );
private:
    SGPropertyNode_ptr _magnitudeNode;
};

typedef simgear::Singleton<Turbulence> TurbulenceSingleton;

class Wind : public PresetBase {
public:
    Wind();
    void preset( double min_hdg, double max_hdg, double speed, double gust );
private:
    SGPropertyNode_ptr _fromNorthNode;
    SGPropertyNode_ptr _fromEastNode;
};

typedef simgear::Singleton<Wind> WindSingleton;

class Visibility : public PresetBase {
public:
    Visibility();
    void preset( double visibility_m );
private:
    SGPropertyNode_ptr _visibilityNode;
};

typedef simgear::Singleton<Visibility> VisibilitySingleton;

} // namespace Presets

} // namespace Environment

#endif //__ENVIRONMENT_PRESETS_HXX
