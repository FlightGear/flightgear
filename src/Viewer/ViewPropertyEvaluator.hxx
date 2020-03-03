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

#pragma once

#include "simgear/props/props.hxx"

#include <map>
#include <ostream>
#include <string>
#include <vector>


namespace ViewPropertyEvaluator {

    /*
    Overview:
    
        We provide efficient evaluation of 'nested properties', where the path
        of the property to be evaluated is determined by the value of other,
        possibly dynamically-changing, properties.

    Details:
    
        Code is expected to specify a nested property as a C string 'spec'
        using (...) to denote evaluation of a substring as a property path
        where required. We use the raw address of C string specs as keys in an
        internal std::map, to provide fast lookup of previously-created specs
        using just a small number of raw pointer comparisons. This requires
        that the C string specs remain valid and unchanged; typically they will
        be immediate strings specifed directly in source code.

        We set up listeners to detect changes to relevant property nodes so
        that we can force re-evaluation of dependent nested properties when
        required. Thus most lookups end up not touching the property system at
        all, and instead access cached values directly.

        For example this:

            SGPropertyNode* node = ViewPropertyEvaluator::getDoubleValue(
                    "((/sim/view[0]/config/root)/position/altitude-ft)"
                    );

        - will behave like this:

            globals->get_props()->getDoubleValue(
                    globals->get_props()->getStringValue(
                            "/sim/view[0]/config/root",
                            true |*create*|
                            )
                            + "/position/altitude-ft",
                    true |*create *|
                    );

        In this example, an internal listener will be set up to detect changes
        to property '/sim/view[0]/config/root'; if this listener has not fired,
        we will use a cached SGPropertyNode_ptr* directly without querying the
        property tree.
        
        Note that with ViewPropertyEvaluator::getDoubleValue, while the final
        SGPropertyNode* is cached, its value is not cached. Instead we always
        call SGPropertyNode::getDoubleValue(). This is in anticipation of
        typical usage where the final double values will change frequently, so
        caching its value becomes less useful.
    
    Missing property nodes:
    
        If a spec (or part of a spec) evaluates to a property node path that
        does not exist, a new property node is created with value "".

        [This allows us to set up a listener for the property node so we can
        detect changes to its value; it might be nice to instead add something
        to Simgear that allows one to listen to creation of a particular path.]
    */
    

    /* Evaluates a spec as a string. The returned reference will be valid for
    ever (until ViewPropertyEvaluator::clear() is called); its value will be
    unchanged until the next time the spec (or a spec that depends on it) is
    evaluated.
    
    For example, getStringValue("/sim/chase-distance-m") will return
    "/sim/chase-distance-m", while getStringValue("(/sim/chase-distance-m)")
    will return "-25" or similar, depending on the aircraft.
    */
    const std::string& getStringValue(const char* spec);

    /* Evaluates a spec as a double. Only makes sense if <spec> has top-level
    "(...)".

    For example, getDoubleValue("(/sim/chase-distance-m)") will return -25.0 or
    similar, depending on the aircraft.
    
    When this function is used, it doesn't install a listener for the top-level
    node, instead it always calls <top-level-node>->getDoubleValue().
    */
    double getDoubleValue(const char* spec, double default_=0);

    /* Similar to getDoubleValue(). */
    bool getBoolValue(const char* spec, bool default_=false);
    
    
    /* Outputs detailed information about all specs that have been seen.
    
    E.g.:
        SG_LOG(SG_VIEW, SG_DEBUG, "ViewPropertyEvaluator:\n" << ViewPropertyEvaluator::Dump());
    */
    struct Dump {};
    std::ostream& operator << (std::ostream& out, const Dump& dump);
    
    struct DumpOne {
        explicit DumpOne(const char* spec);
        const char* _spec;
    };
    std::ostream& operator << (std::ostream& out, const DumpOne& dumpone);
    
    
    /* Clears all internal state. */
    void clear();

}
