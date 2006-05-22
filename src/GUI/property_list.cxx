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

#include STL_STRING
SG_USING_STD(string);
typedef string stdString;      // puObject has a "string" member

#include <Main/fg_os.hxx>      // fgGetKeyModifiers()
#include <Main/fg_props.hxx>

#include "property_list.hxx"


static int nodeNameCompare(const void *ppNode1, const void *ppNode2)
{
    const SGPropertyNode_ptr pNode1 = *(const SGPropertyNode_ptr *)ppNode1;
    const SGPropertyNode_ptr pNode2 = *(const SGPropertyNode_ptr *)ppNode2;

    int diff = strcmp(pNode1->getName(), pNode2->getName());
    return diff ? diff : pNode1->getIndex() - pNode2->getIndex();
}


static string getValueTypeString(const SGPropertyNode *node)
{
    string result;

    if (!node)
        return "unknown";

    SGPropertyNode::Type type = node->getType();
    if (type == SGPropertyNode::UNSPECIFIED)
        result = "unspecified";
    else if (type == SGPropertyNode::NONE)
        result = "none";
    else if (type == SGPropertyNode::BOOL)
        result = "bool";
    else if (type == SGPropertyNode::INT)
        result = "int";
    else if (type == SGPropertyNode::LONG)
        result = "long";
    else if (type == SGPropertyNode::FLOAT)
        result = "float";
    else if (type == SGPropertyNode::DOUBLE)
        result = "double";
    else if (type == SGPropertyNode::STRING)
        result = "string";

    return result;
}




PropertyList::PropertyList(int minx, int miny, int maxx, int maxy, SGPropertyNode *start) :
    puList(minx, miny, maxx, maxy, short(0), 20),
    _curr(start),
    _flags(fgGetNode("/sim/gui/dialogs/property-browser/show-flags", true)),
    _return(0),
    _entries(0),
    _num_entries(0)

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

    for (int j = 0; j < _num_children; j++) {
        if (!_children[j]->nChildren())
            _children[j]->removeChangeListener(this);
    }

    delete[] _entries;
    delete[] _children;
    _entries = 0;
    _children = 0;
}


void PropertyList::handle_select(puObject *list_box)
{
    PropertyList *prop_list = (PropertyList *)list_box->getUserData();
    int selected = list_box->getIntegerValue();
    bool mod_ctrl = fgGetKeyModifiers() & KEYMOD_CTRL;

    if (selected >= 0 && selected < prop_list->_num_entries) {
        const char *src = prop_list->_entries[selected];

        if (prop_list->dotFiles && (selected < 2)) {
            if (!strcmp(src, ".")) {
                if (mod_ctrl)
                    prop_list->toggleFlags();

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
        if (prop_list->dotFiles)
            selected -= 2;

        SGPropertyNode_ptr child = prop_list->_children[selected];
        assert(child);

        // check if it's a directory
        if (child->nChildren()) {
            prop_list->setCurrent(child);
            return;
        }

        // it is a regular property
        if (child->getType() == SGPropertyNode::BOOL && mod_ctrl) {
            child->setBoolValue(!child->getBoolValue());
            prop_list->update();
        } else
            prop_list->publish(child);

    } else {
        // the user clicked on blank screen
        prop_list->update();
    }
}


void PropertyList::update(bool restore_pos)
{
    int pi;
    int i;

    delete_arrays();
    _num_entries = (int)_curr->nChildren();

    // instantiate string objects and add [.] and [..] for subdirs
    if (!_curr->getParent()) {
        _entries = new char*[_num_entries + 1];
        pi = 0;
        dotFiles = false;

    } else {
        _num_entries += 2;    // for . and ..
        _entries = new char*[_num_entries + 1];

        _entries[0] = new char[2];
        strcpy(_entries[0], ".");

        _entries[1] = new char[3];
        strcpy(_entries[1], "..");

        pi = 2;
        dotFiles = true;
    }

    _num_children = _curr->nChildren();
    _children = new SGPropertyNode_ptr[_num_children];
    for (i = 0; i < _num_children; i++)
        _children[i] = _curr->getChild(i);

    qsort(_children, _num_children, sizeof(_children[0]), nodeNameCompare);

    // Make lists of the children's names, values, etc.
    for (i = 0; i < _num_children; i++, pi++) {
        SGPropertyNode *child = _children[i];

        if (child->nChildren() > 0) {
            stdString name = stdString(child->getDisplayName(true)) + '/';
            _entries[pi] = new char[name.size() + 1];
            strcpy(_entries[pi], name.c_str());

        } else {
            _entries[pi] = 0;       // ensure it's 0 before setting intial value
            updateTextForEntry(i);
            child->addChangeListener(this);
        }
    }

    _entries[_num_entries] = 0;

    int top = getTopItem();
    newList(_entries);
    if (restore_pos)
        setTopItem(top);
}


void PropertyList::updateTextForEntry(int index)
{
    assert((index >= 0) && (index < _num_children));
    SGPropertyNode_ptr node = _children[index];

    stdString name = node->getDisplayName(true);
    stdString type = getValueTypeString(node);
    stdString value = node->getStringValue();

    stdString line = name + " = '" + value + "' (" + type;

    if (_flags->getBoolValue()) {
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
        if (node->isTied())
            ext += 'T';
        if (ext.size())
            line += ", " + ext;
    }

    line += ')';

    if (line.size() >= PUSTRING_MAX)
        line.resize(PUSTRING_MAX - 1);

    if (dotFiles)
        index += 2;

    delete[] _entries[index];
    _entries[index] = new char[line.size() + 1];
    strcpy(_entries[index], line.c_str());
}


void PropertyList::valueChanged(SGPropertyNode *nd)
{
    for (int i = 0; i < _num_children; i++)
        if (_children[i] == nd) {
            updateTextForEntry(i);
            return;
        }
}


void PropertyList::setValue(const char *s)
{
    SGPropertyNode *p = fgGetNode(s, false);
    if (p)
        setCurrent(p);
}


