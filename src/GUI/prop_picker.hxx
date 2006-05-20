#ifndef _PROP_PICKER_HXX
#define _PROP_PICKER_HXX

#include <plib/pu.h>

#include <stdio.h>
#include "gui.h"
#include "dialog.hxx"
#include <simgear/props/props.hxx>

// A local alternative name, for use when a variable called "string"
// is in scope - e.g. in classes derived from puInput.
typedef string stdString;


void prop_pickerInit();
void prop_pickerView( puObject * );
void prop_pickerRefresh();
void prop_editOpen(SGPropertyNode *);


class fgPropPicker : public fgPopup, public SGPropertyChangeListener {
public:
    fgPropPicker ( int x, int y, int w, int h, int arrows,
                   SGPropertyNode *, const char *title = "Pick a file" );

    void find_props ( bool restore_slider_pos = false );
    SGPropertyNode *getCurrent () const { return curr; }
    void setCurrent(SGPropertyNode *p) { curr = p; }

    // over-ride the method from SGPropertyNodeListener
    virtual void valueChanged (SGPropertyNode * node);

    puText *proppath;

private:
    static void handle_select ( puObject *b );
    static void input_entered ( puObject *b );
    static void fgPropPickerHandleSlider ( puObject * slider );
    static void fgPropPickerHandleArrow ( puObject *arrow );
    static void fgPropPickerHandleOk ( puObject* b );

    void delete_arrays ();

    // update the text string in the puList using the given node and
    // updating the requested offset. The value of dotFiles is taken
    // into account before the index is applied, i.e this should be
    // an index into 'children' */
    void updateTextForEntry(int index);

    char** files;
    int num_files;
    int arrow_count;

    SGPropertyNode_ptr curr;
    SGPropertyNode_ptr flags;
    SGPropertyNode_ptr* children;
    int num_children;

    // set if we're displaying the . and .. entries at the start of the
    // list
    bool dotFiles;

protected:
    puFrame *frame;
    puListBox *list_box;
    puSlider *slider;
    puOneShot *cancel_button;
    puOneShot *ok_button;
    puArrowButton *down_arrow;
    puArrowButton *up_arrow;

    NewGUI * _gui;
};



class fgPropEdit : public fgPopup {
public:
    fgPropEdit ( SGPropertyNode *node );

    SGPropertyNode *getEditNode() const { return node; }
    void setEditNode(SGPropertyNode *p) { node = p; }

    static void fgPropEditHandleCancel ( puObject *b );
    static void fgPropEditHandleOK ( puObject* b );

protected:
    puFrame *frame;
    puOneShot *cancel_button;
    puOneShot *ok_button;
    SGPropertyNode_ptr node;
    NewGUI * _gui;

public:
    stdString namestring;					// FIXME make setters/getters
    puText *propname;
    puInput *propinput;
};

#endif // _PROP_PICKER_HXX
