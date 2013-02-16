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

#include <string>

#include <plib/puAux.h>
#include <simgear/props/props.hxx>
#include "FGPUIDialog.hxx"


class PropertyList : public puaList, public SGPropertyChangeListener, public GUI_ID {
public:
    PropertyList(int minx, int miny, int maxx, int maxy, SGPropertyNode *);
    ~PropertyList();

    void update (bool restore_slider_pos = false);
    void setCurrent(SGPropertyNode *p);
    SGPropertyNode *getCurrent() const { return _curr; }
    void publish(SGPropertyNode *p) { _return = p; invokeCallback(); }
    void toggleVerbosity() { _verbose = !_verbose; }

    // overridden plib pui methods
    virtual char *getStringValue(void)
    {
        _return_path.clear();
        if (_return)
            _return_path = _return->getPath(true);
        return const_cast<char*>(_return_path.c_str());
    }
    //virtual char *getListStringValue() { return (char *)(_return ? _return->getPath(true) : ""); }
    virtual void setValue(const char *);

    // listener method
    virtual void valueChanged(SGPropertyNode *node);

private:
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
        char **text;
    };

    // update the text string in the puList using the given node and
    // updating the requested offset.
    void updateTextForEntry(NodeData&);
    void delete_arrays();
    static void handle_select(puObject *b);
    static int nodeNameCompare(const void *, const void *);

    SGPropertyNode_ptr _curr;
    SGPropertyNode_ptr _return;

    char **_entries;
    int _num_entries;

    NodeData *_children;
    int _num_children;

    bool _dot_files;      // . and .. pseudo-dirs currently shown?
    bool _verbose;        // show SGPropertyNode flags
    std::string _return_path;
};


#endif // _PROP_PICKER_HXX
