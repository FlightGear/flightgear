/*
  prop_picker.hxx

*/

#include <plib/pu.h>

#include <stdio.h>
#include "gui.h"

void prop_pickerInit();
void prop_pickerView( puObject * );
void prop_pickerRefresh();
void prop_editInit(char * name, char * value );
void prop_editOpen( char * name, char * value );

class fgPropPicker       ;
class fgPropEdit       ;

class fgPropPicker : public puDialogBox
{

  static void handle_select ( puObject *b ) ;
  static void input_entered ( puObject *b ) ;
  static void fgPropPickerHandleSlider ( puObject * slider );
  static void fgPropPickerHandleArrow ( puObject *arrow );
  static void fgPropPickerHandleOk ( puObject* b );

  char** files ;
  char** names ;
  char** values ;
  char*  dflag ;
  int num_files   ;
  int arrow_count ;
  char startDir [ PUSTRING_MAX * 2 ] ;

/* puInput   *input         ; */

protected:

  puFrame   *frame         ;
  puListBox *list_box      ;
  puSlider  *slider        ;
  puOneShot *cancel_button ;
  puOneShot *ok_button     ;


public:
  puText    *proppath      ;
  void find_props () ;
  fgPropPicker ( int x, int y, int w, int h, int arrows,
                 const char *dir, const char *title = "Pick a file" ) ;

  ~fgPropPicker () {;}

  static void go_up_one_directory ( char *fname ) ;
  static void chop_file ( char *fname ) ;

} ;

class fgPropEdit : public puDialogBox
{

  static void fgPropEditHandleCancel ( puObject *b ) ;
  static void fgPropEditHandleOK ( puObject* b );

protected:

  puFrame   *frame         ;
  puOneShot *cancel_button ;
  puOneShot *ok_button     ;

public:
  puText    *propname     ;
  puInput   *propinput     ;
  char propPath [ PUSTRING_MAX * 2 ] ;

  fgPropEdit ( char *name, char *value, char *proppath ) ;

  ~fgPropEdit () {;}

  static void go_up_one_directory ( char *fname ) ;
  static void chop_file ( char *fname ) ;

} ;

