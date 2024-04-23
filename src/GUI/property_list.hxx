/*
 * SPDX-FileName: property_list.hxx
 * SPDX-FileComment: Implementation of the <property-list> widget.
 * SPDX-FileCopyrightText: Copyright (C) 2001  Steve BAKER
 * SPDX-FileCopyrightText: Copyright (C) 2001  Jim WILSON
 * SPDX-FileCopyrightText: Copyright (C) 2006  Melchior FRANZ
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>

// ensure we include this before puAux.h, so that 
// #define _PU_H_ 1 has been done, and hence we don't
// include the un-modified system pu.h
#include "FlightGear_pu.h"

#include <plib/puAux.h>
#include <simgear/props/props.hxx>
#include "FGPUIDialog.hxx"


class PropertyList : public puaList, public SGPropertyChangeListener, public GUI_ID {
public:
    // If <readonly> is true, we don't show . or .. items and don't respond to
    // mouse clicks or keypresses.
    PropertyList(int minx, int miny, int maxx, int maxy, SGPropertyNode *root, bool readonly);
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
    bool _readonly;
};


