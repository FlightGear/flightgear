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

#include "ViewPropertyEvaluator.hxx"

#include "Main/globals.hxx"

#include <algorithm>
#include <cassert>


namespace ViewPropertyEvaluator {

    /* We represent a spec as graph, using alternating Sequence and Node
    objects so that different specs share common information; e.g. this ensures
    that we don't install more than one listener for the same SGPropertyNode.
    
    Evaluating top-level nodes:

        Currently ViewPropertyEvaluator::getDoubleValue() will always
        reevaluate the top-level SGPropertyNode by calling its getDoubleValue()
        member. This usually gives the desired behaviour because most
        final property nodes that we are used with don't appear to make
        valueChanged() callbacks, and it's anyway probably more efficient to
        not use such callbacks for rapidly-changing values.

        However it would be good to be clearer about this, e.g. maybe we could
        have a second bracket notation to indicate that we should evaluate and
        cache the SGPropertyNode but not its string/double value. E.g.:

            ViewPropertyEvaluator::getDoubleValue(
                    "{(/sim/view[0]/config/root)/position/altitude-ft}"
                    );

        - would not attempt to install a valueChanged() callback for the
        top-level SGPropertyNode.
*/
    
    struct Sequence;
    struct Node;
    
    struct Sequence
    {
        Sequence();

        std::vector<std::shared_ptr<Node>>  _nodes;
        std::vector<Node*>                  _parents;
        bool                                _rescan;
        std::string                         _value;
    };

    struct Node : SGPropertyChangeListener
    {
        explicit Node(const char* spec);

        /* SGPropertyChangeListener callback. */
        void valueChanged(SGPropertyNode* node);
        
        const char*                 _begin;
        const char*                 _end;
        bool                        _rescan;
        std::string                 _value;

        std::vector<Sequence*>      _parents;

        // Only used if _begin.._end is (...).
        std::shared_ptr<Sequence>   _child;
        SGPropertyNode_ptr          _sgnode;
        SGPropertyNode_ptr          _sgnode_listen;
    };

    /* Helper for dumping a Sequence to an ostream. Prefixes all lines with
    <indent>. If <deep> is true, recursively shows all child sequences and
    nodes. */
    struct SequenceDump
    {
        SequenceDump(const Sequence& sequence, const std::string& indent="", bool deep=false);
        
        const Sequence&     _sequence;
        const std::string&  _indent;
        bool                _deep;
        
        friend std::ostream& operator << (std::ostream& out, const SequenceDump& self);
    };
    
    /* Helper for dumping a Node to an ostream. Prefixes all lines with
    <indent>. If <deep> is true, recursively shows all child sequences and
    nodes. */
    struct NodeDump
    {
        NodeDump(const Node& node, const std::string& indent="", bool deep=false);
        
        const Node&         _node;
        const std::string&  _indent;
        bool                _deep;
        
        friend std::ostream& operator << (std::ostream& out, const NodeDump& self);
    };
    
    /* Support for debug statistics. */
    struct Debug
    {
        /* Support for tracking how many property system accesses we are making. */
        struct Stat
        {
            Stat() : n(0) {}
            int n;
        };
    
        /* Increments counter for <name>. Periodically outputs stats with
        SG_LOG(SG_VIEW, SG_DEBUG, ...) and detailed information about Sequences
        and Nodes with SG_LOG(SG_VIEW, SG_BULK, ...). */
        void    statsAdd(const char* name);
        void    statsReset();
        struct StatsShow {};
        friend std::ostream& operator << (std::ostream& out, const StatsShow&);
        
        /* Track how many listeners we have. */
        void listensAdd(SGPropertyNode_ptr node);
        void listensRemove(SGPropertyNode_ptr node);
        
        time_t  statsT0 = 0;
        std::map<std::string, std::shared_ptr<Stat>>    stats;
        std::vector<SGPropertyNode_ptr> listens;
    };
    
    Debug   debug;
    
    /* Forces this node and all of its sequence and node parents to be
    re-read the next time they are evaluated - e.g. the next call of
    getNodeStringValue() will call getSequenceStringValue() on _child and
    write the result into the <_value> member before returning <_value>. */
    void rescanNode(Node& node);

    /* Forces this sequence and all of its node and sequence parents to
    be re-read the next time they are evaluated - e.g. the next call of
    getSequenceStringValue() will call getNodeStringValue() on each child
    node and concatenate the results into the <_value> member before
    returning <_value>. */
    void rescanSequence(Sequence& sequence);

    /* Returns Sequence for spec starting at <spec>, which can be subsequently
    used to evaluate the spec efficiently. We require that the string <spec>
    will remain unchanged forever. */
    std::shared_ptr<Sequence>   getSequence(const char* spec);

    /* Returns evaluated spec for <node> using caching. Mainly used
    internally.

    If node spec is of the form "<...>", we look up the value in the
    property system. */
    const std::string& getNodeStringValue(Node& node);

    /* Returns evaluated spec for <sequence> using caching. Mainly used
    internally. Concatenates the string value of each child node. */
    const std::string& getSequenceStringValue(Sequence& sequence);

    /* <sequence> must be from a spec with a top-level '(...)'. Returns the
    specified property-tree node. */
    SGPropertyNode* getSequenceNode(Sequence& sequence);

    double getSequenceDoubleValue(Sequence& sequence, double default_=0);

    /* Finds end of section of spec starting at <spec>, to be used as the
    region of a Sequence; this will contain one or more regions corresponding
    to a Node. Any ')' without a corresponding '(' is treated as a terminator.
    */
    const char* getSequenceEnd(const char* spec)
    {
        int nesting = 0;
        for (const char* s=spec; ; ++s) {
            if (*s == 0) {
                assert(nesting == 0);
                return s;
            }
            if (*s == '(') {
                nesting += 1;
            }
            if (*s == ')') {
                if (nesting == 0) {
                    return s;
                }
                nesting -= 1;
            }
        }
    }

    /* Finds end of section of spec starting at <spec>, to be used as the
    region of a Node.

    We parse things similarly to getSequenceEnd() except that we also terminate
    early in the following situations:

        The first character is not '(' and we find a '('.

        The first character is '(' and we find the corresponding ')'.
    */
    const char* getNodeEnd(const char* spec)
    {
        int nesting = 0;
        for (const char* s=spec; ; ++s) {
            if (*s == 0) {
                assert(nesting == 0);
                return s;
            }
            if (*s == '(') {
                if (spec[0] != '(') {
                    return s;
                }
                nesting += 1;
            }
            if (*s == ')') {
                if (nesting == 0) {
                    return s;
                }
                nesting -= 1;
                if (spec[0] == '(' && nesting == 0) {
                    return s + 1;
                }
            }
        }
    }

    /* Raw constructor. We only set up basic values; the rest is done
    by getNodeInternal(). */
    Node::Node(const char* spec)
    :
    _begin(spec),
    _end(getNodeEnd(_begin)),
    _rescan(true)
    {
    }

    void Node::valueChanged(SGPropertyNode* node)
    {
        debug.statsAdd("valueChanged");
        SG_LOG(SG_VIEW, SG_DEBUG, "valueChanged():"
                << " node->_sgnode_listen->getPath()='" << _sgnode_listen->getPath() << "'"
                << " node->getPath()='" << node->getPath() << "'"
                );
        rescanNode(*this);
    }
    
    Sequence::Sequence()
    :
    _rescan(true)
    {
    }
    
    void rescanNode(Node& node)
    {
        node._rescan = true;
        for (auto sequence: node._parents) {
            rescanSequence(*sequence);
        }
    }

    void rescanSequence(Sequence& sequence)
    {
        sequence._rescan = true;
        for (auto node: sequence._parents) {
            rescanNode(*node);
        }
    }

    /* Caches of various structures so that we can look things up quickly.
    */

    /* This is the main cache. It uses raw C string pointers (pointing
    to specs) as keys, so lookups only require a handful of pointer
    comparisons. */
    std::map<const char*, std::shared_ptr<Sequence>>    spec_to_sequence;

    /* These are only used when parsing new specs and creating new nodes and
    sequencies, so are not speed-critical. */
    std::map<std::string, std::shared_ptr<Sequence>>    string_to_sequence;
    std::map<std::string, std::shared_ptr<Node>>        string_to_node;

    
    /* Finds or creates new Sequence for (possibly initial) portion of <spec>.
    */
    std::shared_ptr<Sequence>   getSequenceInternal(const char* spec, Node* parent);
    
    /* Finds or creates new Node for (possibly initial) portion of <spec>.
    */
    std::shared_ptr<Node>       getNodeInternal(const char* spec, Sequence* parent);

    std::shared_ptr<Sequence>   getSequenceInternal(const char* spec, Node* parent)
    {
        if (spec[0] == 0 || spec[0] == ')') {
            return NULL;
        }

        std::shared_ptr<Sequence>   sequence;

        std::string spec_string(spec, getSequenceEnd(spec));
        auto it = string_to_sequence.find(spec_string);
        if (it == string_to_sequence.end()) {
            sequence.reset(new Sequence);
            for(const char* s = spec;;) {
                std::shared_ptr<Node>   node
                        = getNodeInternal(s, sequence.get() /*parent*/);
                if (!node)  break;
                sequence->_nodes.push_back(node);
                s += (node->_end - node->_begin);
            }
            string_to_sequence[spec_string] = sequence;
        }
        else {
            sequence = it->second;
        }

        if (parent) {
            auto it = std::find(
                    sequence->_parents.begin(),
                    sequence->_parents.end(),
                    parent
                    );
            if (it == sequence->_parents.end()) {
                sequence->_parents.push_back(parent);
            }
        }
        return sequence;
    }
    
    std::shared_ptr<Node>   getNodeInternal(const char* spec, Sequence* parent)
    {
        if (spec[0] == 0 || spec[0] == ')') {
            return NULL;
        }

        std::shared_ptr<Node>   node;

        std::string s(spec, getNodeEnd(spec));
        auto it = string_to_node.find(s);

        if (it == string_to_node.end()) {
            node.reset(new Node(spec));
            if (node->_begin[0] == '(') {
                node->_child = getSequenceInternal(node->_begin + 1, node.get() /*parent*/);
            }
            string_to_node[s] = node;
        }
        else {
            node = it->second;
        }

        if (parent) {
            if (std::find(node->_parents.begin(), node->_parents.end(), parent)
                    == node->_parents.end()) {
                node->_parents.push_back(parent);
            }
        }

        return node;
    }

    /* Finds or creates new Sequence for (possibly initial) portion of <spec>
    and sets spec_to_sequence[spec] so that it can be quickly looked up in
    future. */
    std::shared_ptr<Sequence>   getSequence(const char* spec)
    {
        auto it = spec_to_sequence.find(spec);
        if (it != spec_to_sequence.end()) {
            return it->second;
        }

        std::shared_ptr<Sequence>   sequence
                = getSequenceInternal(spec, NULL /*parent*/);
        spec_to_sequence[spec] = sequence;
        SG_LOG(SG_VIEW, SG_DEBUG,
                "Created new sequence:\n"
                << SequenceDump(*sequence, "    ", true /*deep*/)
                );
        return sequence;
    }

    /* Evaluates <node> to find path and returns corresponding SGPropertyNode*
    in global properties. If <cache> is true, we install a listener on
    the returned node, so that we force a rescan of all the node's parent
    Sequence's if its value changes. */
    SGPropertyNode* getNodeSGNode(Node& node, bool cache=true)
    {
        assert(node._begin[0] == '(');
        assert(node._child);
        if (node._rescan) {
            node._rescan = false;
            if (!node._sgnode || node._child->_rescan) {
                const std::string& path = getSequenceStringValue(*node._child);
                SGPropertyNode* sgnode = NULL;
                if (path != "") {
                    debug.statsAdd( "propertypath_getNode");
                    sgnode = globals->get_props()->getNode(path, true /*create*/);
                    if (!sgnode) {
                        debug.statsAdd( "propertypath_getNode_failed");
                        SG_LOG(SG_VIEW, SG_DEBUG, ": getNodeSGNode(): getNode() failed, path='" << path << "'");
                    }
                }
                if (sgnode != node._sgnode) {
                    if (node._sgnode_listen) {
                        node._sgnode_listen->removeChangeListener(&node);
                        debug.listensRemove(node._sgnode_listen);
                        node._sgnode_listen = NULL;
                    }
                    if (node._sgnode) {
                        node._sgnode->removeChangeListener(&node);
                    }
                    node._sgnode = sgnode;
                    if (node._sgnode && cache) {
                        node._sgnode_listen = node._sgnode;
                        node._sgnode->addChangeListener(&node, false /*initial*/);
                        debug.listensAdd(node._sgnode);
                    }
                }
                if (!node._sgnode && path != "") {
                    /* Ideally we'd ask Simgear's property system to call us
                    back if <path> was created, but this is non-trivial, and
                    not actually required when we are used by view.cxx. */
                }
            }
        }
        return node._sgnode;
    }
    
    const std::string& getNodeStringValue(Node& node)
    {
        if (node._rescan) {
            if (node._begin[0] == '(') {
                getNodeSGNode(node);
                if (node._sgnode) {
                    debug.statsAdd( "property_getStringValue");
                    node._value = node._sgnode->getStringValue();
                }
                else {
                    node._value = "";
                }
            }
            else {
                node._rescan = false;
                node._value = std::string(node._begin, node._end);
            }
        }

        return node._value;
    }

    const std::string& getSequenceStringValue(Sequence& sequence)
    {
        if (sequence._rescan) {
            sequence._rescan = false;
            sequence._value = "";
            for (auto node: sequence._nodes) {
                sequence._value += getNodeStringValue(*node);
            }
        }
        return sequence._value;
    }

    /* Assumes that sequence has a single child Node object with a non-empty
    Sequence child object whose value is a path in the property system. */
    SGPropertyNode* getSequenceNode(Sequence& sequence)
    {
        assert(sequence._nodes.size() == 1);
        Node&   node = *sequence._nodes.front();
        return getNodeSGNode(node, false /*cache*/);
    }

    /* This is different from getSequenceStringValue() in that it assumes that
    a spec has a single top-level (...) and so the top-level sequence has a
    single child Node object which in turn has a Sequence child object whose
    value is a path in the property system.

    We always call the SGPropertyNode's getDoubleValue() method - we don't
    cache the double value. But the underlying SGPropertyNode* will be cached.
    */
    double getSequenceDoubleValue(Sequence& sequence, double default_)
    {
        SGPropertyNode* node = getSequenceNode(sequence);
        if (!node) {
            return default_;
        }
        if (!node->getParent()) {
            /* Root node. */
            return default_;
        }
        if (node->getType() == simgear::props::BOOL) {
            /* 2020-03-22: there is a problem with aircraft rah-66 setting type
            of the root node to bool which gives string value "false". So we
            force a return of default_. */
            return default_;
        }
        if (node->getStringValue()[0] == 0) {
            /* If we reach here, the node exists but its value is an empty
            string, so node->getDoubleValue() would return 0 which isn't
            useful, so instead we return default_. */
            return default_;
        }
        return node->getDoubleValue();
    }

    bool getSequenceBoolValue(Sequence& sequence, bool default_)
    {
        SGPropertyNode* node = getSequenceNode(sequence);
        if (node) {
            if (node->getStringValue()[0] != 0) {
                return node->getBoolValue();
            }
        }
        return default_;
    }

    const std::string& getStringValue(const char* spec)
    {
        std::shared_ptr<Sequence>   sequence = getSequence(spec);
        return getSequenceStringValue(*sequence);
    }

    double getDoubleValue(const char* spec, double default_)
    {
        std::shared_ptr<Sequence>   sequence = getSequence(spec);
        if (sequence->_nodes.size() != 1 || sequence->_nodes.front()->_begin[0] != '(') {
            SG_LOG(SG_VIEW, SG_DEBUG,  "bad sequence for getDoubleValue() - must have outermost '(...)': '" << spec);
            abort();
        }
        double ret = getSequenceDoubleValue(*sequence, default_);
        return ret;
    }

    bool getBoolValue(const char* spec, bool default_)
    {
        std::shared_ptr<Sequence>   sequence = getSequence(spec);
        if (sequence->_nodes.size() != 1 || sequence->_nodes.front()->_begin[0] != '(') {
            SG_LOG(SG_VIEW, SG_DEBUG,  "bad sequence for getBoolValue() - must have outermost '(...)': '" << spec);
            abort();
        }
        bool ret = getSequenceBoolValue(*sequence, default_);
        return ret;
    }

    std::ostream& operator << (std::ostream& out, const Dump& dump)
    {
        out << "ViewPropertyEvaluator\n";
        out << "    Number of specs: " << spec_to_sequence.size() << ":\n";
        int i = 0;
        for (auto it: spec_to_sequence) {
            out << "        " << (i+1) << "/" << spec_to_sequence.size() << ": spec: " << it.first << "\n";
            out << SequenceDump(*it.second, "        ", true /*deep*/);
            i += 1;
        }
        out << "    Number of sequences: " << string_to_sequence.size() <<  "\n";
        i = 0;
        for (auto it: string_to_sequence) {
            out << "        " << (i+1) << "/" << string_to_sequence.size()
                    << ": spec='" << it.first << "'"
                    << ": " << SequenceDump(*it.second, "", false /*deep*/)
                    ;
            i += 1;
        }
        out << "    Number of nodes: " << string_to_node.size() << "\n";
        i = 0;
        for (auto it: string_to_node) {
            out << "        " << (i+1) << "/" << string_to_node.size()
                    << ": spec='" << it.first << "'"
                    << ": " << NodeDump(*it.second, "", false /*deep*/)
                    ;
            i += 1;
        }
        out << "    Number of listens: " << debug.listens.size() << "\n";
        i = 0;
        for (auto it: debug.listens) {
            out << "        " << (i+1) << "/" << debug.listens.size()
                    << ": " << it->getPath()
                    << "\n";
            i += 1;
        }
        return out;
    }
    
    DumpOne::DumpOne(const char* spec)
    : _spec(spec)
    {}
    
    std::ostream& operator << (std::ostream& out, const DumpOne& dumpone)
    {
        out << "ViewPropertyEvaluator\n";
        std::shared_ptr<Sequence> sequence = getSequence(dumpone._spec);
        if (sequence) {
            out << "    " << ": spec: '" << dumpone._spec << "'\n";
            out << SequenceDump(*sequence, "        ", true /*deep*/);
        }
        return out;
    }
    
    void clear()
    {
        spec_to_sequence.clear();
        string_to_sequence.clear();
        string_to_node.clear();
        
        debug = Debug();
    }
    
    /* === Everything below here is for diagnostics and/or debugging. */
    
    SequenceDump::SequenceDump(const Sequence& sequence, const std::string& indent, bool deep)
    :
    _sequence(sequence),
    _indent(indent),
    _deep(deep)
    {}
    
    NodeDump::NodeDump(const Node& node, const std::string& indent, bool deep)
    :
    _node(node),
    _indent(indent),
    _deep(deep)
    {}
    
    
    void Debug::listensAdd(SGPropertyNode_ptr node)
    {
        debug.listens.push_back(node);
    }
    
    void Debug::listensRemove(SGPropertyNode_ptr node)
    {
        auto it = std::find(debug.listens.begin(), debug.listens.end(), node);
        if (it == debug.listens.end()) {
            SG_LOG(SG_VIEW, SG_ALERT, "Unable to find node in debug.listens");
        }
        else {
            debug.listens.erase(it);
        }
    }
    
    std::ostream& operator << (std::ostream& out, const Debug::StatsShow&)
    {
        time_t t = time(NULL);
        time_t dt = t - debug.statsT0;
        if (dt == 0)    dt = 1;
        out << "ViewPropertyEvaluator stats: dt=" << dt << "\n";
        for (auto it: debug.stats) {
            const std::string&  name = it.first;
            int n = it.second->n;
            out << "    : n=" << n << " n/sec=" << (1.0 * n / dt) << ": " << name << "\n";
        }
        return out;
    }
    
    void    Debug::statsReset() {
        for (auto it: debug.stats) {
            it.second->n = 0;
        }
        debug.statsT0 = time(NULL);
    }
    
    void    Debug::statsAdd(const char* name) {
        if (debug.statsT0 == 0)  debug.statsT0 = time(NULL);
        std::shared_ptr<Debug::Stat>&   stat = debug.stats[name];
        if (!stat) {
            stat.reset(new(Debug::Stat));
        }
        stat->n += 1;
        
        if (1) {
            static time_t  t0 = time(NULL);
            time_t  t = time(NULL);
            if (t - t0 > 10) {
                t0 = t;
                SG_LOG(SG_VIEW, SG_DEBUG, StatsShow());
                statsReset();
                
                /* Output all specs with SG_BULK. */
                SG_LOG(SG_VIEW, SG_BULK, Dump());
            }
        }
    }
    
    std::ostream& operator << (std::ostream& out, const SequenceDump& self)
    {
        std::string spec;
        for (auto node: self._sequence._nodes) {
            spec += std::string(node->_begin, node->_end);
        }

        out << self._indent
                << "Sequence at " << &self._sequence << ":"
                << " _rescan=" << self._sequence._rescan
                << " _parents.size()=" << self._sequence._parents.size()
                << " _nodes.size()=" << self._sequence._nodes.size()
                << " _value='" << self._sequence._value << "'"
                << " spec='" << spec << "'"
                << "\n";
        if (self._deep) {
            for (auto node: self._sequence._nodes) {
                out << NodeDump(*node, self._indent + "    ", self._deep);
            }
        }
        return out;
    }
    
    std::ostream& operator << (std::ostream& out, const NodeDump& self)
    {
        out << self._indent
                << "Node at " << &self._node << ":"
                << " _rescan=" << self._node._rescan
                << " _parents.size()=" << self._node._parents.size()
                << " _child=" << self._node._child.get()
                << " _value='" << self._node._value << "'"
                << " _sgnode=" << self._node._sgnode
                ;
        if (self._node._sgnode) {
            out
                    << " (path=" << self._node._sgnode->getPath()
                    << ", value=" << self._node._sgnode->getStringValue()
                    << ")";
        }
        out
                << " _begin.._end='" << std::string(self._node._begin, self._node._end) << "'"
                << "\n";
        if (self._deep) {
            if (self._node._child) {
                out << SequenceDump(*self._node._child, self._indent + "    ", self._deep);
            }
        }
        return out;
    }
    
    void dump()
    {
        std::cerr << Dump() << "\n";
    }
}
