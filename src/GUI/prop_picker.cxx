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

#include "prop_picker.hxx"
#include <Main/globals.hxx>

#include <simgear/compiler.h>
#include <simgear/misc/props.hxx>

#define DOTDOTSLASH "../"
#define SLASH       "/"

string name, line, value;

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

void prop_editInit(char * name, char * value, char * proppath)
{
	if( PE_widget == 0 ) {
            fgPropEdit *PP = new fgPropEdit ( name, value, proppath );
	    PE_widget = PP;
	}
}

void prop_editOpen( char * name, char * value, char * proppath )
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
static string getValueTypeString( const SGPropertyNode *node ) {
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
  float val ;
  slider -> getValue ( &val ) ;
  val = 1.0f - val ;

  puListBox* list_box = (puListBox*) slider -> getUserData () ;
  int index = int ( list_box -> getNumItems () * val ) ;
  list_box -> setTopItem ( index ) ;
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
  int num_items = list_box->getNumItems () - 1 ;
  if ( num_items > 0 )
  {
    int index = int ( num_items * val + 0.5 ) + inc ;
    if ( index > num_items ) index = num_items ;
    if ( index < 0 ) index = 0 ;

    slider -> setValue ( 1.0f - (float)index / num_items ) ;
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

    if ( strcmp ( src, "." ) == 0 )
    {
      /* Do nothing - but better refresh anyway. */

      prop_picker -> find_props () ;
      return ;
    } 

    if ( strcmp ( src, ".." ) == 0 )
    {
      /* Do back up one level - so refresh. */

      go_up_one_directory ( dst ) ;
      prop_picker -> find_props () ;
      return ;
    } 

    if ( prop_picker -> dflag [ selected ] )
    {
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
    prop_editOpen( prop_picker -> names [ selected ], prop_picker -> values [ selected ], dst);
  }
  else
  {
    /*
      The user clicked on blank screen - maybe we should
      refresh just in case some other process created the
      file.
    */

    prop_picker -> find_props () ;
  }
}


void fgPropPicker::fgPropPickerHandleOk ( puObject* b )
{
  fgPropPicker* prop_picker = (fgPropPicker*) b -> getUserData () ;

  /* nothing to do, just hide */
  FG_POP_PUI_DIALOG( prop_picker );
}

/*

fgPropPicker::~fgPropPicker ()
{
  if ( files )
  {
    for ( int i=0; i<num_files; i++ ) {
      delete files[i];
      delete names[i];
      delete values[i];
    }

    delete[] files;
    delete[] names;
    delete[] values;
    delete[] dflag;
  }

  if ( this == puActiveWidget () )
    puDeactivateWidget () ;
}

*/

fgPropPicker::fgPropPicker ( int x, int y, int w, int h, int arrows,
                                      const char *dir, const char *title ) : puDialogBox ( x,y )
{
  puFont LegendFont, LabelFont;
  puGetDefaultFonts ( &LegendFont, &LabelFont );

  files = NULL ;
  dflag = NULL ;
  names = NULL ;
  values = NULL ;
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
  slider->setDelta(0.1f);
  slider->setValue(1.0f);
  slider->setSliderFraction (0.2f) ;
  slider->setCBMode( PUSLIDER_DELTA );
  
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

  if ( arrows == 2 )
  {
    down_arrow = new puArrowButton ( w-30, 40, w-10, 60, PUARROW_FASTDOWN ) ;
    down_arrow->setUserData ( slider ) ;
    down_arrow->setCallback ( fgPropPickerHandleArrow ) ;

    up_arrow = new puArrowButton ( w-30, h-80, w-10, h-60, PUARROW_FASTUP ) ;
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
  printf("fgPropPicker - End of Init\n");
}


void fgPropPicker::find_props ()
{

  int pi;

  if ( files != NULL )
  {
    for ( int i = 0 ; i < num_files ; i++ ) {
      delete files[i] ;
      delete names[i] ;
      delete values[i] ;
    }

    delete [] files ;
    delete [] names ;
    delete [] values ;
    delete [] dflag ;
  }

  num_files = 0 ;

  char dir [ PUSTRING_MAX * 2 ] ;

  int  iindex = 0;
  char sindex [ 20 ];

  strcpy ( dir, startDir ) ;

  int i = 0 ;
//  printf("dir begin of find_props=%s\n",dir);
//  printf("len of dir=%i",strlen(dir));
  SGPropertyNode * node = globals->get_props()->getNode(dir);

  num_files = (int)node->nChildren();

  // instantiate string objects and add [.] and [..] for subdirs
  if (strcmp(dir,"/") == 0) {
    files = new char* [ num_files+1 ] ;
    names = new char* [ num_files+1 ] ;
    values = new char* [ num_files+1 ] ;
    dflag = new char  [ num_files+1 ] ;
    pi = 0;
  } else {
    // add two for the .. and .
    num_files = num_files + 2;
    // make room for .. and .
    files = new char* [ num_files+1 ] ;
    names = new char* [ num_files+1 ] ;
    values = new char* [ num_files+1 ] ;
    dflag = new char  [ num_files+1 ] ;
    line = ".";
    files [ 0 ] = new char[ strlen(line.c_str())+2 ];
    strcpy ( files [ 0 ], line.c_str() );
    names [ 0 ] = new char[ 2 ];
    values[ 0 ] = new char[ 2 ] ;
    line = "..";
    dflag[ 0 ] = 1;
    files [ 1 ] = new char[ strlen(line.c_str())+2 ];
    strcpy ( files [ 1 ], line.c_str() );
    names [ 1 ] = new char[ strlen(line.c_str())+2 ];
    values[ 1 ] = new char[ 2 ] ;
    strcpy ( names [ 1 ], line.c_str() );

    dflag[ 1 ] = 1;
    pi = 2;
  };


  for (i = 0; i < (int)node->nChildren(); i++) {
	    SGPropertyNode * child = node->getChild(i);
	    name = child->getName();
	    if ( node->getChild(name.c_str(), 1) != 0 ) {
		iindex = child->getIndex();
		sprintf(sindex, "[%d]", iindex);
	        name += sindex;
	    }
	    line = name;
  	    names[ pi ] = new char[ strlen(line.c_str())+2 ] ;
	    strcpy ( names [ pi ], line.c_str() ) ;
	    if ( child->nChildren() > 0 ) {
		dflag[ pi ] = 1 ;
                files[ pi ] = new char[ strlen(line.c_str())+strlen(sindex)+4 ] ;
	        strcpy ( files [ pi ], line.c_str() ) ;
	        strcat ( files [ pi ], "/" ) ;
   	        values[ pi ] = new char[ 2 ] ;
	    } else {
                dflag[ pi ] = 0 ;
		value = node->getStringValue ( name.c_str(), "" );
   	        values[ pi ] = new char[ strlen(value.c_str())+2 ] ;
                strcpy ( values [pi], value.c_str() );
		line += " = '" + value + "' " + "(";
		line += getValueTypeString( node->getNode( name.c_str() ) );
                line += ")";
                files[ pi ] = new char[ strlen(line.c_str())+2 ] ;
	        strcpy ( files [ pi ], line.c_str() ) ;
	    }
            // printf("files->%i of %i %s\n",pi, node->nChildren(), files [pi]);
            ++pi;
  }

  // truncate entries to 80 characters (plib pui limit)
  for (i = 0; i < num_files; i++) {
    if (strlen(files[i]) > 80) files[i][79] = '\0';
  }

  files [ num_files ] = NULL ;

  // leave the . and .. alone...
  int ii = ( strcmp(files [0], "." ) == 0 ) ? 2 : 0;

  // Sort the entries.  This is a simple N^2 extraction sort.  More
  // elaborate algorithms aren't necessary for the few dozen
  // properties we're going to sort.
  for(i=ii; i<num_files; i++) {
    int j, min = i;
    char df, *tmp;
    for(j=i+1; j<num_files; j++)
      if(strcmp(names[j], names[min]) < 0)
	min = j;
    if(i != min) {
      tmp =  names[min];  names[min] =  names[i];  names[i] = tmp;
      tmp =  files[min];  files[min] =  files[i];  files[i] = tmp;
      tmp = values[min]; values[min] = values[i]; values[i] = tmp;
      df  =  dflag[min];  dflag[min] =  dflag[i];  dflag[i] = df;
    }
  }

  // printf("files pointer=%i/%i\n", files, num_files);

  proppath ->    setLabel          (startDir);

  list_box -> newList ( files ) ;

  // if non-empty list, adjust the size of the slider...
  if (num_files > 1) {
    if ((11.0f/(num_files)) < 1) {
      slider->setSliderFraction (11.0f/(num_files)) ;
      slider->reveal();
      up_arrow->reveal();
      down_arrow->reveal();
      }
    else {
      slider->setSliderFraction (0.9999f) ;
      slider->hide();
      up_arrow->hide();
      down_arrow->hide();      
      }
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

fgPropEdit::fgPropEdit ( char *name, char *value, char *proppath ) : puDialogBox ( 0, 0 )

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

