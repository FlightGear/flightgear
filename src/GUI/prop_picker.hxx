#ifndef _PROP_PICKER_HXX
#define _PROP_PICKER_HXX

#include <plib/pu.h>

#include <stdio.h>
#include "gui.h"
#include "dialog.hxx"
#include <simgear/props/props.hxx>

void prop_pickerInit();
void prop_pickerView( puObject * );
void prop_pickerRefresh();
void prop_editOpen( const char * name, const char * value );


class fgPropPicker;
class fgPropEdit;

class fgPropPicker : public fgPopup, public SGPropertyChangeListener {

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
    char startDir [ PUSTRING_MAX * 2 ];

    SGPropertyNode_ptr* children;
    int num_children;

    // set if we're display the . and .. entries at the start of the
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

public:
    puText *proppath;
    void find_props ( bool restore_slider_pos = false );
    fgPropPicker ( int x, int y, int w, int h, int arrows,
                   const char *dir, const char *title = "Pick a file" );

    ~fgPropPicker () {}

    static void go_up_one_directory ( char *fname );
    static void chop_file ( char *fname );

    // over-ride the method from SGPropertyNodeListener
    virtual void valueChanged (SGPropertyNode * node);
};



class fgPropEdit : public fgPopup {

  static void fgPropEditHandleCancel ( puObject *b );
  static void fgPropEditHandleOK ( puObject* b );

protected:
  puFrame *frame;
  puOneShot *cancel_button;
  puOneShot *ok_button;

  NewGUI * _gui;

public:
  puText *propname;
  puInput *propinput;
  char propPath [ PUSTRING_MAX * 2 ];

  fgPropEdit ( const char *name, const char *value, char *proppath );

  ~fgPropEdit () {}

  static void go_up_one_directory ( char *fname );
  static void chop_file ( char *fname );
};

#endif // _PROP_PICKER_HXX
