/*

     Adapted by Jim Wilson, beginning Sept 2001 (FG v 0.79)

****     Insert FlightGear GPL here.

     Based on puFilePicker from:

     ********
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id$
     ********
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include STL_STRING

#include <Main/globals.hxx>

#include "prop_picker.hxx"

SG_USING_STD(string);

// A local alternative name, for use when a variable called "string" is in scope - e.g. in classes derived from puInput.
typedef string stdString;

#define DOTDOTSLASH "../"
#define SLASH       "/"

static puObject *PP_widget = 0;

// widget location and size...
#define PROPPICK_X 100
#define PROPPICK_Y 200
#define PROPPICK_W 500
#define PROPPICK_H 300

static puObject *PE_widget = 0;

void prop_pickerInit()
{
	if( PP_widget == 0 ) {
            fgPropPicker *PP = new fgPropPicker ( PROPPICK_X, PROPPICK_Y, PROPPICK_W, PROPPICK_H, 1, "/", "FG Properties");
	    PP_widget = PP;
	}
}

void prop_pickerView( puObject * )
{
	if( PP_widget == 0 ) {
		prop_pickerInit();
	}
	fgPropPicker *me = (fgPropPicker *)PP_widget -> getUserData();
        // refresh
        me -> find_props();
	FG_PUSH_PUI_DIALOG( me );
}

void prop_pickerRefresh()
{
	if( PP_widget == 0 ) {
		prop_pickerInit();
	}
	fgPropPicker *me = (fgPropPicker *)PP_widget -> getUserData();
	me -> find_props();
}

void prop_editInit( const char * name, const char * value, char * proppath )
{
	if( PE_widget == 0 ) {
            fgPropEdit *PP = new fgPropEdit ( name, value, proppath );
	    PE_widget = PP;
	}
}

void prop_editOpen( const char * name, const char * value, char * proppath )
{
	if( PE_widget == 0 ) {
		prop_editInit( name, value, proppath );
	}
	fgPropEdit *me = (fgPropEdit *)PE_widget -> getUserData();
        me -> propname ->    setLabel          (name);
        me -> propinput ->   setValue          (value);
        strcpy(me -> propPath, proppath);
        me -> propinput -> acceptInput ();
	FG_PUSH_PUI_DIALOG( me );
}

// return a human readable form of the value "type"
static string getValueTypeString( const SGPropertyNode_ptr node ) {
    string result;

    if ( node == NULL ) {
	return "unknown";
    }

    SGPropertyNode::Type type = node->getType();
    if ( type == SGPropertyNode::UNSPECIFIED ) {
	result = "unspecified";
    } else if ( type == SGPropertyNode::NONE ) {
        result = "none";
    } else if ( type == SGPropertyNode::BOOL ) {
	result = "bool";
    } else if ( type == SGPropertyNode::INT ) {
	result = "int";
    } else if ( type == SGPropertyNode::LONG ) {
	result = "long";
    } else if ( type == SGPropertyNode::FLOAT ) {
	result = "float";
    } else if ( type == SGPropertyNode::DOUBLE ) {
	result = "double";
    } else if ( type == SGPropertyNode::STRING ) {
	result = "string";
    }

    return result;
}

void fgPropPicker::fgPropPickerHandleSlider ( puObject * slider )
{
  puListBox* list_box = (puListBox*) slider -> getUserData () ;

  float val ;
  slider -> getValue ( &val ) ;
  val = 1.0f - val ;
  int scroll_range = list_box->getNumItems () - list_box->getNumVisible() ;
  if ( scroll_range > 0 )
  {
    int index = int ( scroll_range * val + 0.5 ) ;
    list_box -> setTopItem ( index ) ;
  }
}

void fgPropPicker::fgPropPickerHandleArrow ( puObject *arrow )
{
  puSlider *slider = (puSlider *) arrow->getUserData () ;
  puListBox* list_box = (puListBox*) slider -> getUserData () ;

  int type = ((puArrowButton *)arrow)->getArrowType() ;
  int inc = ( type == PUARROW_DOWN     ) ?   1 :
            ( type == PUARROW_UP       ) ?  -1 :
            ( type == PUARROW_FASTDOWN ) ?  10 :
            ( type == PUARROW_FASTUP   ) ? -10 : 0 ;

  float val ;
  slider -> getValue ( &val ) ;
  val = 1.0f - val ;
  int scroll_range = list_box->getNumItems () - list_box->getNumVisible() ;
  if ( scroll_range > 0 )
  {
    int index = int ( scroll_range * val + 0.5 ) ;
    index += inc ;
    // if ( index > scroll_range ) index = scroll_range ;
    // Allow buttons to scroll further than the slider does
    if ( index > ( list_box->getNumItems () - 1 ) )
      index = ( list_box->getNumItems () - 1 ) ;
    if ( index < 0 ) index = 0 ;

    slider -> setValue ( 1.0f - (float)index / scroll_range ) ;
    list_box -> setTopItem ( index ) ;
  }
}


void fgPropPicker::chop_file ( char *fname )
{
  /* removes everything back to the last '/' */

  for ( int i = strlen(fname)-1 ; fname[i] != SLASH[0] && i >= 0 ; i-- )
    fname[i] = '\0' ;
}


void fgPropPicker::go_up_one_directory ( char *fname )
{
  /* removes everything back to the last but one '/' */

  chop_file ( fname ) ;

  if ( strlen ( fname ) == 0 )
  {
    /* Empty string!  The only way to go up is to append a "../" */

    strcpy ( fname, DOTDOTSLASH ) ;
    return ;
  }

  /* If the last path element is a "../" then we'll have to add another "../" */

  if ( strcmp ( & fname [ strlen(fname)-3 ], DOTDOTSLASH ) == 0 )
  {
   if ( strlen ( fname ) + 4 >= PUSTRING_MAX )
    {
      ulSetError ( UL_WARNING, "PUI: fgPropPicker - path is too long, max is %d.",
                          PUSTRING_MAX ) ;
      return ;
    }

    strcat ( fname, DOTDOTSLASH ) ;
    return ;
  }

  /* Otherwise, just delete the last element of the path. */

  /* Remove the trailing slash - then remove the rest as if it was a file name */

  fname [ strlen(fname)-1 ] = '\0' ;
  chop_file ( fname ) ;
}


void fgPropPicker::handle_select ( puObject* list_box )
{
  fgPropPicker* prop_picker = (fgPropPicker*) list_box -> getUserData () ;
  int selected ;
  list_box -> getValue ( &selected ) ;

  if ( selected >= 0 && selected < prop_picker -> num_files )
  {
    char *dst = prop_picker -> startDir ;
    char *src = prop_picker -> files [ selected ] ;

    if (prop_picker->dotFiles && (selected < 2)) {
        if ( strcmp ( src, "." ) == 0 ) {
            /* Do nothing - but better refresh anyway. */
	
            prop_picker -> find_props () ;
            return ;
        } else if ( strcmp ( src, ".." ) == 0 ) {
            /* Do back up one level - so refresh. */
	
            go_up_one_directory ( dst ) ;
            prop_picker -> find_props () ;
            return ;
        }
    }

    //  we know we're dealing with a regular entry, so convert
    // it to an index into children[]
    if (prop_picker->dotFiles) selected -= 2; 
    SGPropertyNode_ptr child = prop_picker->children[selected];
    assert(child != NULL);
		
    // check if it's a directory (had children)
    if ( child->nChildren() ) {
      /* If this is a directory - then descend into it and refresh */

      if ( strlen ( dst ) + strlen ( src ) + 2 >= PUSTRING_MAX )
      {
	ulSetError ( UL_WARNING,
          "PUI: fgPropPicker - path is too long, max is %d.", PUSTRING_MAX ) ;
	return ;
      }

      strcat ( dst, src ) ; /* add path to descend to */
      prop_picker -> find_props () ;
      return ;
    }

    /* If this is a regular file - then just append it to the string */

    if ( strlen ( dst ) + strlen ( src ) + 2 >= PUSTRING_MAX )
    {
      ulSetError ( UL_WARNING, 
        "PUI: fgPropPicker - path is too long, max is %d.", PUSTRING_MAX ) ;
      return ;
    }
	
    prop_editOpen(child->getName(), child->getStringValue(), dst);
  }
  else
  {
    /*
      The user clicked on blank screen - maybe we should
      refresh just in case some other process created the
      file.
    */
      // should be obsolete once we observe child add/remove on our top node
      prop_picker -> find_props () ;
  }
}


void fgPropPicker::fgPropPickerHandleOk ( puObject* b )
{
  fgPropPicker* prop_picker = (fgPropPicker*) b -> getUserData () ;

  /* nothing to do, just hide */
  FG_POP_PUI_DIALOG( prop_picker );
}


void fgPropPicker::delete_arrays ()
{
  if ( files )
  {
    for ( int i=0; i<num_files; i++ ) {
		delete[] files[i];
    }

    for (int C=0; C<num_children; ++C) {
        if (children[C]->nChildren() == 0)
            children[C]->removeChangeListener(this);
    }
	
    delete[] files;
    delete[] children;
  }
}

/*

fgPropPicker::~fgPropPicker ()
{
  delete_arrays();

  if ( this == puActiveWidget () )
    puDeactivateWidget () ;
}

*/

fgPropPicker::fgPropPicker ( int x, int y, int w, int h, int arrows,
                                      const char *dir, const char *title ) : fgPopup ( x,y )
{
  puFont LegendFont, LabelFont;
  puGetDefaultFonts ( &LegendFont, &LabelFont );

  files = NULL ;
  num_files = 0 ;

  strcpy ( startDir, dir ) ;
  // printf ( "StartDirLEN=%i", strlen(startDir));
  if ( arrows > 2 ) arrows = 2 ;
  if ( arrows < 0 ) arrows = 0 ;
  arrow_count = arrows ;

  frame = new puFrame ( 0, 0, w, h );

  setUserData( this );

  proppath = new puText            (10, h-30);
  proppath ->    setLabel          (startDir);

  slider = new puSlider (w-30,40+20*arrows,h-100-40*arrows,TRUE,20);
  slider->setValue(1.0f);
  
  list_box = new puListBox ( 10, 40, w-40, h-60 ) ;
  list_box -> setLabel ( title );
  list_box -> setLabelPlace ( PUPLACE_ABOVE ) ;
  list_box -> setStyle ( -PUSTYLE_SMALL_SHADED ) ;
  list_box -> setUserData ( this ) ;
  list_box -> setCallback ( handle_select ) ;
  list_box -> setValue ( 0 ) ;

  ok_button = new puOneShot ( 10, 10, (w<170)?(w/2-5):80, 30 ) ;
  ok_button -> setLegend ( "Ok" ) ;
  ok_button -> setUserData ( this ) ;
  ok_button -> setCallback ( fgPropPickerHandleOk ) ;

  if ( arrows > 0 )
  {
    down_arrow = new puArrowButton ( w-30, 20+20*arrows, w-10, 40+20*arrows, PUARROW_DOWN ) ;
    down_arrow->setUserData ( slider ) ;
    down_arrow->setCallback ( fgPropPickerHandleArrow ) ;

    up_arrow = new puArrowButton ( w-30, h-60-20*arrows, w-10, h-40-20*arrows, PUARROW_UP ) ;
    up_arrow->setUserData ( slider ) ;
    up_arrow->setCallback ( fgPropPickerHandleArrow ) ;
  }

  // after picker is built, load the list box with data...
  find_props () ;

//  printf("after Props files[1]=%s\n",files[1]);
//  printf("num items %i", list_box -> getNumItems ());

  slider -> setUserData ( list_box ) ;
  slider -> setCallback ( fgPropPickerHandleSlider ) ;

  FG_FINALIZE_PUI_DIALOG( this );
}


// Like strcmp, but for sorting property nodes into a suitable display order.
static int nodeNameCompare(const void *ppNode1, const void *ppNode2)
{
  const SGPropertyNode_ptr pNode1 = *(const SGPropertyNode_ptr *)ppNode1;
  const SGPropertyNode_ptr pNode2 = *(const SGPropertyNode_ptr *)ppNode2;

  // Compare name first, and then index.
  int diff = strcmp(pNode1->getName(), pNode2->getName());
  if (diff) return diff;
  return pNode1->getIndex() - pNode2->getIndex();
}


// Replace the current list of properties with the children of node "startDir".
void fgPropPicker::find_props ()
{
  int pi;
  int i;

  delete_arrays();
  num_files = 0 ;

//  printf("dir begin of find_props=%s\n",startDir);
//  printf("len of dir=%i",strlen(startDir));
  SGPropertyNode * node = globals->get_props()->getNode(startDir);

  num_files = (node) ? (int)node->nChildren() : 0;
		
  // instantiate string objects and add [.] and [..] for subdirs
  if (strcmp(startDir,"/") == 0) {
    files = new char* [ num_files+1 ] ;
    pi = 0;
    dotFiles = false;
  } else {
    // add two for the .. and .
    num_files = num_files + 2;
    // make room for .. and .
    files = new char* [ num_files+1 ] ;

    stdString line = ".";
    files [ 0 ] = new char[line.size() + 1];
    strcpy ( files [ 0 ], line.c_str() );

    line = "..";
    files [ 1 ] = new char[line.size() + 1];
    strcpy ( files [ 1 ], line.c_str() );

    pi = 2;
    dotFiles = true;
  };


  if (node) {
    // Get the list of children
    num_children = node->nChildren();
    children = new SGPropertyNode_ptr[num_children];
    for (i = 0; i < num_children; i++) {
      children[i] = node->getChild(i);
    }
	
    // Sort the children into display order
    qsort(children, num_children, sizeof(children[0]), nodeNameCompare);

    // Make lists of the children's names, values, etc.
    for (i = 0; i < num_children; i++) {
        SGPropertyNode * child = children[i];
        stdString name = child->getDisplayName(true);

        if ( child->nChildren() > 0 ) {
            files[ pi ] = new char[ strlen(name.c_str())+2 ] ;
            strcpy ( files [ pi ], name.c_str() ) ;
            strcat ( files [ pi ], "/" ) ;
        } else {
            files[pi] = NULL; // ensure it's NULL before setting intial value
            updateTextForEntry(i);
            // observe it
            child->addChangeListener(this);
        }
		
        ++pi;
    }
  }

  files [ num_files ] = NULL ;

  proppath ->    setLabel          (startDir);

  list_box -> newList ( files ) ;

  // adjust the size of the slider...
  if (num_files > list_box->getNumVisible()) {
    slider->setSliderFraction((float)list_box->getNumVisible() / num_files);
    slider->setValue(1.0f);
    slider->reveal();
    up_arrow->reveal();
    down_arrow->reveal();
  } else {
    slider->hide();
    up_arrow->hide();
    down_arrow->hide();      
  }
}

void fgPropPicker::updateTextForEntry(int index)
{
    assert((index >= 0) && (index < num_children));
    SGPropertyNode_ptr node = children[index];
	
    // take a copy of the value	
    stdString value = node->getStringValue();

    stdString line = node->getDisplayName() + stdString(" = '")
        + value + "' " + "(";
    line += getValueTypeString( node );
    line += ")";
	
    // truncate entries to plib pui limit
    if (line.length() >= PUSTRING_MAX)
        line[PUSTRING_MAX-1] = '\0';

    if (dotFiles) index +=2;
		
    // don't leak everywhere if we're updating
    if (files[index]) delete[] files[index];
		
    files[index] = new char[ strlen(line.c_str())+2 ] ;
    strcpy ( files [ index ], line.c_str() ) ;
}

void fgPropPicker::valueChanged(SGPropertyNode *nd)
{
    for (int C=0; C<num_children; ++C)
        if (children[C] == nd) {
            updateTextForEntry(C);
            return;
        }
}

void fgPropEdit::fgPropEditHandleCancel ( puObject* b )
{
  fgPropEdit* prop_edit = (fgPropEdit*) b -> getUserData () ;
  FG_POP_PUI_DIALOG( prop_edit );
}

void fgPropEdit::fgPropEditHandleOK ( puObject* b )
{
  fgPropEdit* prop_edit = (fgPropEdit*) b -> getUserData () ;
  const char* tname;
  char* tvalue;

  // use label text for property node to be updated
  tname = prop_edit -> propname -> getLabel();
  prop_edit -> propinput -> getValue( &tvalue );

  SGPropertyNode * node = globals->get_props()->getNode(prop_edit -> propPath);
  node->getNode( prop_edit -> propname -> getLabel(), true)->setStringValue(tvalue);

  // update the picker display so it shows new value
  prop_pickerRefresh();

  FG_POP_PUI_DIALOG( prop_edit );
}

fgPropEdit::fgPropEdit ( const char *name, const char *value, char *proppath ) : fgPopup ( 0, 0 )

{
    puFont LegendFont, LabelFont;
    puGetDefaultFonts ( &LegendFont, &LabelFont );

    // locate in relation to picker widget...
    int fx = PROPPICK_X;
    int fy = PROPPICK_Y + PROPPICK_H;
    frame   = new puFrame           (fx,fy, fx+500, fy+120);

    strcpy (propPath, proppath);

    setUserData( this );

    propname = new puText            (fx+10, fy+90);
    propname ->    setLabel          (name);
        
    propinput   = new puInput           (fx+10, fy+50, fx+480, fy+80);
    propinput   ->    setValue          (value);
    propinput   ->    acceptInput();
        

    ok_button     =  new puOneShot   (fx+10, fy+10, fx+80, fy+30);
    ok_button     ->     setUserData (this);
    ok_button     ->     setLegend   (gui_msg_OK);
    ok_button     ->     setCallback (fgPropEditHandleOK);
    ok_button     ->     makeReturnDefault(TRUE);
        
    cancel_button =  new puOneShot   (fx+100, fy+10, fx+180, fy+30);
    cancel_button     ->     setUserData (this);
    cancel_button ->     setLegend   (gui_msg_CANCEL);
    cancel_button ->     setCallback (fgPropEditHandleCancel);
        
    FG_FINALIZE_PUI_DIALOG( this );
}
