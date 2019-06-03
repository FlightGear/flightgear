// Copyright (C) 2019 James Turner <james@flightgear.org>
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

#ifndef FG_ABSTRACT_INSTRUMENT_HXX
#define FG_ABSTRACT_INSTRUMENT_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

class AbstractInstrument : public SGSubsystem
{
public:

protected:
    
    void readConfig(SGPropertyNode* config,
                    std::string defaultName);
    
    void initServicePowerProperties(SGPropertyNode* node);

    bool isServiceableAndPowered() const;

    // build the path /instrumentation/<name>[number]
    std::string nodePath() const;
    
    std::string name() const { return _name; }
    int number() const { return _index; }
    
    void unbind() override;
    
    void setMinimumSupplyVolts(double v);
    
    /**
     * specify the default path to use to power the instrument, if it's non-
     * standard.
     */
    void setDefaultPowerSupplyPath(const std::string &p);

    virtual bool isPowerSwitchOn() const;
private:
    std::string _name;
    int _index = 0;
    std::string _powerSupplyPath;
    
    SGPropertyNode_ptr _serviceableNode;
    SGPropertyNode_ptr _powerButtonNode;
    double _minimumSupplyVolts;
    SGPropertyNode_ptr _powerSupplyNode;
};

#endif // of FG_ABSTRACT_INSTRUMENT_HXX
