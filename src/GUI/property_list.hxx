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

#ifndef _PROPERTY_LIST_HXX
#define _PROPERTY_LIST_HXX


#include <plib/pu.h>
#include <simgear/props/props.hxx>
#include "dialog.hxx"

#include "puList.hxx"

class PropertyList : public puList, public SGPropertyChangeListener, public GUI_ID {
public:
    PropertyList(int minx, int miny, int maxx, int maxy, SGPropertyNode *);
    ~PropertyList();

    void update (bool restore_slider_pos = false);
    void setCurrent(SGPropertyNode *p);
    SGPropertyNode *getCurrent() const { return _curr; }
    void publish(SGPropertyNode *p) { _return = p; invokeCallback(); }
    void toggleFlags() { _flags->setBoolValue(!_flags->getBoolValue()); }

    // overridden plib pui methods
    virtual char *getListStringValue() { return (char *)(_return ? _return->getPath(true) : ""); }
    //virtual char *getStringValue(void) { return (char *)(_return ? _return->getPath(true) : ""); }
    virtual void setValue(const char *);

    // listener method
    virtual void valueChanged(SGPropertyNode *node);

private:
    // update the text string in the puList using the given node and
    // updating the requested offset. The value of dotFiles is taken
    // into account before the index is applied, i.e this should be
    // an index into 'children'
    void updateTextForEntry(int index);
    void delete_arrays();
    static void handle_select(puObject *b);
    static int nodeNameCompare(const void *, const void *);

    SGPropertyNode_ptr _curr;
    SGPropertyNode_ptr _flags;
    SGPropertyNode_ptr _return;

    char **_entries;
    int _num_entries;

    struct NodeData {
        NodeData() : listener(0) {}
        ~NodeData() {
            if (listener)
                node->removeChangeListener(listener);
        }
        void setListener(SGPropertyChangeListener *l) {
            node->addChangeListener(listener = l);
        }
        SGPropertyNode_ptr node;
        SGPropertyChangeListener *listener;
    };
    NodeData *_children;
    int _num_children;

    bool dotFiles;      // . and .. pseudo-dirs currently shown?
};


#endif // _PROP_PICKER_HXX
