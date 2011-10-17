// Implementation of the <property-list> widget.
//
// Copyright (C) 2001  Steve BAKER
// Copyright (C) 2001  Jim WILSON
// Copyright (C) 2006  Melchior FRANZ
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
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>

using std::string;
using std::cout;
using std::endl;

typedef string stdString;      // puObject has a "string" member

#include <Main/fg_os.hxx>      // fgGetKeyModifiers()
#include <Main/fg_props.hxx>

#include "property_list.hxx"


static string getValueTypeString(const SGPropertyNode *node)
{
    using namespace simgear;
    string result;

    props::Type type = node->getType();
    if (type == props::UNSPECIFIED)
        result = "unspecified";
    else if (type == props::NONE)
        result = "none";
    else if (type == props::BOOL)
        result = "bool";
    else if (type == props::INT)
        result = "int";
    else if (type == props::LONG)
        result = "long";
    else if (type == props::FLOAT)
        result = "float";
    else if (type == props::DOUBLE)
        result = "double";
    else if (type == props::STRING)
        result = "string";
    else if (type == props::VEC3D)
        result = "vec3d";
    else if (type == props::VEC4D)
        result = "vec4d";

    return result;
}


static void dumpProperties(const SGPropertyNode *node)
{
    using namespace simgear;
    cout << node->getPath() << '/' << endl;
    for (int i = 0; i < node->nChildren(); i++) {
        const SGPropertyNode *c = node->getChild(i);
        props::Type type = c->getType();
        if (type == props::ALIAS || c->nChildren())
            continue;

        int index = c->getIndex();
        cout << std::setw(11) << getValueTypeString(c) << "  " << c->getName();
        if (index > 0)
            cout << '[' << index << ']';
        cout << " = ";

        switch (c->getType()) {
        case props::DOUBLE:
        case props::FLOAT:
        case props::VEC3D:
        case props::VEC4D:
        {
            std::streamsize precision = cout.precision(15);
            c->printOn(cout);
            cout.precision(precision);
        }
            break;
        case props::LONG:
        case props::INT:
        case props::BOOL:
            c->printOn(cout);
            break;
        case props::STRING:
            cout << '"' << c->getStringValue() << '"';
            break;
        case props::NONE:
            break;
        default:
            cout << '\'' << c->getStringValue() << '\'';
        }
        cout << endl;
    }
    cout << endl;
}


static void sanitize(stdString& s)
{
    stdString r = s;
    s = "";
    for (unsigned i = 0; i < r.size(); i++) {
        if (r[i] == '\a')
            s += "\\a";
        else if (r[i] == '\b')
            s += "\\b";
        else if (r[i] == '\t')
            s += "\\t";
        else if (r[i] == '\n')
            s += "\\n";
        else if (r[i] == '\v')
            s += "\\v";
        else if (r[i] == '\f')
            s += "\\f";
        else if (r[i] == '\r')
            s += "\\r";
        else if (r[i] == '\'')
            s += "\'";
        else if (r[i] == '\\')
            s += "\\";
        else if (isprint(r[i]))
            s += r[i];
        else {
            const char *hex = "0123456789abcdef";
            int c = r[i] & 255;
            s += stdString("\\x") + hex[c / 16] + hex[c % 16];
        }
    }
}




PropertyList::PropertyList(int minx, int miny, int maxx, int maxy, SGPropertyNode *start) :
    puaList(minx, miny, maxx, maxy, short(0), 20),
    GUI_ID(FGCLASS_PROPERTYLIST),
    _curr(start),
    _return(0),
    _entries(0),
    _num_entries(0),
    _verbose(false)

{
    _list_box->setUserData(this);
    _list_box->setCallback(handle_select);
    _list_box->setValue(0);
    update();
}


PropertyList::~PropertyList()
{
    delete_arrays();
}


void PropertyList::delete_arrays()
{
    if (!_entries)
        return;

    for (int i = 0; i < _num_entries; i++)
        delete[] _entries[i];

    delete[] _entries;
    delete[] _children;
    _entries = 0;
    _children = 0;
}


void PropertyList::handle_select(puObject *list_box)
{
    PropertyList *prop_list = (PropertyList *)list_box->getUserData();
    int selected = list_box->getIntegerValue();
    int mod_ctrl = fgGetKeyModifiers() & KEYMOD_CTRL;
    int mod_shift = fgGetKeyModifiers() & KEYMOD_SHIFT;

    if (selected >= 0 && selected < prop_list->_num_entries) {
        const char *src = prop_list->_entries[selected];

        if (prop_list->_dot_files && (selected < 2)) {
            if (src[0] == '.' && (src[1] == '\0' || src[1] == ' ')) {
                if (mod_ctrl && mod_shift)
                    prop_list->_curr->fireValueChanged();
                else if (mod_ctrl)
                    prop_list->toggleVerbosity();
                else if (mod_shift)
                    dumpProperties(prop_list->_curr);

                prop_list->update();
                return;

            } else if (!strcmp(src, "..")) {
                SGPropertyNode *parent = prop_list->getCurrent()->getParent();
                if (parent) {
                    if (mod_ctrl)
                        for (; parent->getParent(); parent = parent->getParent())
                            ;
                    prop_list->setCurrent(parent);
                }
                return;
            }
        }

        // we know we're dealing with a regular entry, so convert
        // it to an index into children[]
        if (prop_list->_dot_files)
            selected -= 2;

        SGPropertyNode_ptr child = prop_list->_children[selected].node;

        // check if it's a directory
        if (child->nChildren()) {
            prop_list->setTopItem(0);
            prop_list->setCurrent(child);
            return;
        }

        // it is a regular property
        if (child->getType() == simgear::props::BOOL && mod_ctrl) {
            child->setBoolValue(!child->getBoolValue());
            prop_list->update(true);
        } else
            prop_list->publish(child);

    } else {
        // the user clicked on blank screen
        prop_list->update(true);
    }
}


void PropertyList::update(bool restore_pos)
{
    int pi;

    delete_arrays();
    _num_entries = (int)_curr->nChildren();

    // instantiate string objects and add [.] and [..] for subdirs
    if (!_curr->getParent()) {
        _entries = new char*[_num_entries + 1];
        pi = 0;
        _dot_files = false;

    } else {
        _num_entries += 2;    // for . and ..
        _entries = new char*[_num_entries + 1];

        _entries[0] = new char[16];
        strcpy(_entries[0], _verbose ? ".     [verbose]" : ".");

        _entries[1] = new char[3];
        strcpy(_entries[1], "..");

        pi = 2;
        _dot_files = true;
    }

    int i;
    _num_children = _curr->nChildren();
    _children = new NodeData[_num_children];
    for (i = 0; i < _num_children; i++)
        _children[i].node = _curr->getChild(i);

    qsort(_children, _num_children, sizeof(_children[0]), nodeNameCompare);

    // Make lists of the children's names, values, etc.
    for (i = 0; i < _num_children; i++, pi++) {
        _children[i].text = &_entries[pi];
        _entries[pi] = 0;    // make it deletable
        updateTextForEntry(_children[i]);
        _children[i].setListener(this);
    }

    _entries[_num_entries] = 0;

    int top = getTopItem();
    newList(_entries);
    if (restore_pos)
        setTopItem(top);
}


void PropertyList::updateTextForEntry(NodeData& data)
{
    using namespace simgear;
    SGPropertyNode *node = data.node;
    stdString name = node->getDisplayName(true);
    stdString type = getValueTypeString(node);
    stdString value = node->getStringValue();

    std::ostringstream line;
    line << name;

    int children = node->nChildren();
    if (children)
        line << '/';

    if (!children || (_verbose && node->hasValue())) {
        if (node->getType() == props::STRING
                || node->getType() == props::UNSPECIFIED)
            sanitize(value);

        line << " = '" << value << "' (" << type;

        if (_verbose) {
            stdString ext;
            if (!node->getAttribute(SGPropertyNode::READ))
                ext += 'r';
            if (!node->getAttribute(SGPropertyNode::WRITE))
                ext += 'w';
            if (node->getAttribute(SGPropertyNode::TRACE_READ))
                ext += 'R';
            if (node->getAttribute(SGPropertyNode::TRACE_WRITE))
                ext += 'W';
            if (node->getAttribute(SGPropertyNode::ARCHIVE))
                ext += 'A';
            if (node->getAttribute(SGPropertyNode::USERARCHIVE))
                ext += 'U';
            if (node->getAttribute(SGPropertyNode::PRESERVE))
                ext += 'P';
            if (node->isTied())
                ext += 'T';

            if (!ext.empty())
                line << ", " << ext;

            int num = node->nListeners();
            if (data.listener)
                num--;
            if (num)
                line << ", L" << num;
        }
        line << ')';
    }
    else
    if ((_verbose)&&(node->getAttribute(SGPropertyNode::PRESERVE)))
    {
        // only preserve/protection flag matters for nodes without values
        line << " (P)";
    }

    stdString out = line.str();
    if (out.size() >= PUSTRING_MAX)
        out.resize(PUSTRING_MAX - 1);

    delete[] *data.text;
    *data.text = new char[out.size() + 1];
    strcpy(*data.text, out.c_str());
}


void PropertyList::valueChanged(SGPropertyNode *nd)
{
    for (int i = 0; i < _num_children; i++)
        if (_children[i].node == nd) {
            updateTextForEntry(_children[i]);
            return;
        }
}


int PropertyList::nodeNameCompare(const void *p1, const void *p2)
{
    const SGPropertyNode *n1 = (*(const NodeData *)p1).node;
    const SGPropertyNode *n2 = (*(const NodeData *)p2).node;

    int diff = strcmp(n1->getName(), n2->getName());
    return diff ? diff : n1->getIndex() - n2->getIndex();
}


void PropertyList::setValue(const char *s)
{
    try {
        SGPropertyNode *p = fgGetNode(s, false);
        if (p)
            setCurrent(p);
        else
            throw stdString("node doesn't exist");
    } catch (const stdString& m) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "property-list node '" << s << "': " << m);
    }
}


void PropertyList::setCurrent(SGPropertyNode *p)
{
    bool same = (_curr == p);
    _return = _curr = p;
    update(same);
    if (!same)
        publish(p);
}

