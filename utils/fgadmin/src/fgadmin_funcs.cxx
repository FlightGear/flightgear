// fgadmin_funcs.cxx -- FG Admin UI functions.
//
// Written by Curtis Olson, started February 2004.
//
// Copyright (c) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>

#ifdef _MSC_VER
#  include <direct.h>
#endif

#include <FL/Fl_File_Chooser.H>
#include <plib/ul.h>

#include <simgear/misc/sg_path.hxx>

#include "fgadmin.h"
#include "untarka.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;

extern string def_install_source;
extern string def_scenery_dest;

static const float min_progress = 0.0;
static const float max_progress = 5000.0;

// destructor
FGAdminUI::~FGAdminUI() {
    delete prefs;
}


// initialize additional class elements
void FGAdminUI::init() {
    prefs = new Fl_Preferences( Fl_Preferences::USER,
                                "flightgear.org",
                                "fgadmin" );
    char buf[FL_PATH_MAX];
    prefs->get( "install-source", buf, def_install_source.c_str(), FL_PATH_MAX );
    source_text->value( buf );
    source = buf;

    prefs->get( "scenery-dest", buf, def_scenery_dest.c_str(), FL_PATH_MAX );
    dest_text->value( buf );
    dest = buf;

    refresh_lists();

    progress->minimum( min_progress );
    progress->maximum( max_progress );

    main_window->size_range( 465, 435 );
}

// show our UI
void FGAdminUI::show() {
    main_window->show();
}


// refresh the check box lists
void FGAdminUI::refresh_lists() {
    update_install_box();
    update_remove_box();
}


// scan the source directory and update the install_box contents
// quit
void FGAdminUI::quit() {
    main_window->hide();
}


// select the install source
void FGAdminUI::select_install_source() {
    char* p = fl_dir_chooser( "Select Scenery Source Directory",
                              source.c_str(),
                              0 );
    if ( p != 0 ) {
	source = p;
        source_text->value( p );
        prefs->set( "install-source", p );
        prefs->flush();
    }
}


// select the install source
void FGAdminUI::select_install_dest() {
    char* p = fl_dir_chooser( "Select Scenery Install Directory",
                              dest.c_str(),
                              0 );
    if ( p != 0 ) {
	dest = p;
        dest_text->value( p );
        prefs->set( "scenery-dest", p );
        prefs->flush();
    }
}


// Like strcmp, but for strings
static int stringCompare(const void *string1, const void *string2)
{
    const string *s1 = (const string *)string1;
    const string *s2 = (const string *)string2;

    // Compare name first, and then index.
    return strcmp(s1->c_str(), s2->c_str());
}


void FGAdminUI::update_install_box() {
    int i;
    vector<string> file_list;
    file_list.clear();

    install_box->clear();

    if ( source.length() ) {
        ulDir *dir = ulOpenDir( source.c_str() ) ;
        ulDirEnt *ent;
        while ( dir != 0 && ( ent = ulReadDir( dir ) ) ) {
            // find base name of archive file
            char base[FL_PATH_MAX];
            strncpy( base, ent->d_name, FL_PATH_MAX );
            const char *p = fl_filename_ext( base );
            int offset, expected_length = 0;
            if ( strcmp( p, ".gz" ) == 0 ) {
                offset = p - base;
                base[offset] = '\0';
                p = fl_filename_ext( base );
                if ( strcmp( p, ".tar" ) == 0 ) {
                    offset = p - base;
                    base[offset] = '\0';
                    expected_length = 14;
                }
            } else if ( strcmp( p, ".tgz" ) == 0 ) {
                offset = p - base;
                base[offset] = '\0';
                expected_length = 11;
            } else if ( strcmp( p, ".zip" ) == 0 ) {
                offset = p - base;
                base[offset] = '\0';
                expected_length = 11;
            }

            if ( strlen(ent->d_name) != expected_length ) {
                // simple heuristic to ignore non-scenery files
            } else if ( ent->d_name[0] != 'e' && ent->d_name[0] != 'w' ) {
                // further sanity checks on name
            } else if ( ent->d_name[4] != 'n' && ent->d_name[4] != 's' ) {
                // further sanity checks on name
            } else {
                // add to list if not installed
                SGPath install( dest );
                install.append( base );
                if ( ! fl_filename_isdir( install.c_str() ) ) {
                    // cout << install.str() << " install candidate." << endl;
                    file_list.push_back( ent->d_name );
                } else {
                    // cout << install.str() << " exists." << endl;
                }
            }
        }
        ulCloseDir( dir );

        // convert to a qsort()'able array
        string *sort_list = new string[file_list.size()];
        for ( i = 0; i < file_list.size(); ++i ) {
            sort_list[i] = fl_filename_name( file_list[i].c_str() );
        }

        // Sort the file list into display order
        qsort(sort_list, file_list.size(), sizeof(string), stringCompare);

        for ( int i = 0; i < file_list.size(); ++i ) {
            install_box->add( sort_list[i].c_str() );
        }

        delete [] sort_list;

        install_box->redraw();
    }
}


// scan the source directory and update the install_box contents
void FGAdminUI::update_remove_box() {
    int i;
    vector<string> dir_list;
    dir_list.clear();

    remove_box->clear();

    if ( dest.length() ) {
        ulDir *dir = ulOpenDir( dest.c_str() ) ;
        ulDirEnt *ent;
        while ( dir != 0 && ( ent = ulReadDir( dir ) ) ) {
            if ( strlen(ent->d_name) != 7 ) {
                // simple heuristic to ignore non-scenery directories
            } else if ( ent->d_name[0] != 'e' && ent->d_name[0] != 'w' ) {
                // further sanity checks on name
            } else if ( ent->d_name[4] != 'n' && ent->d_name[4] != 's' ) {
                // further sanity checks on name
            } else {
                dir_list.push_back( ent->d_name );
            }
        }
        ulCloseDir( dir );

        // convert to a qsort()'able array
        string *sort_list = new string[dir_list.size()];
        for ( i = 0; i < dir_list.size(); ++i ) {
            sort_list[i] = fl_filename_name( dir_list[i].c_str() );
        }

        // Sort the file list into display order
        qsort(sort_list, dir_list.size(), sizeof(string), stringCompare);

        for ( int i = 0; i < dir_list.size(); ++i ) {
            remove_box->add( sort_list[i].c_str() );
        }

        delete [] sort_list;

        remove_box->redraw();
    }
}


// install selected files
void FGAdminUI::install_selected() {
    char *f;

    install_b->deactivate();
    remove_b->deactivate();
    quit_b->deactivate();

    // traverse install box and install each item
    for ( int i = 0; i <= install_box->nitems(); ++i ) {
        if ( install_box->checked( i ) ) {
            f = install_box->text( i );
            SGPath file( source );
            file.append( f );
            struct stat info;
            stat( file.str().c_str(), &info );
            float old_max = progress->maximum();
            progress->maximum( info.st_size );
            progress_label = "Installing ";
            progress_label += f;
            progress->label( progress_label.c_str() );
            progress->value( min_progress );
            main_window->cursor( FL_CURSOR_WAIT );
            tarextract( (char *)file.c_str(), (char *)dest.c_str(), true, &FGAdminUI::step, this );
            progress->value( min_progress );
            main_window->cursor( FL_CURSOR_DEFAULT );
            progress->label( "" );
            progress->maximum( old_max );
        }
    }
    quit_b->activate();
    install_b->activate();
    remove_b->activate();

    refresh_lists();
}


static unsigned long count_dir( const char *dir_name ) {
    ulDir *dir = ulOpenDir( dir_name ) ;
    ulDirEnt *ent;
    unsigned long cnt = 0L;
    while ( ent = ulReadDir( dir ) ) {
        if ( strcmp( ent->d_name, "." ) == 0 ) {
            // ignore "."
        } else if ( strcmp( ent->d_name, ".." ) == 0 ) {
            // ignore ".."
        } else if ( ent->d_isdir ) {
            SGPath child( dir_name );
            child.append( ent->d_name );
            cnt += count_dir( child.c_str() );
        } else {
            cnt += 1;
        }
    }
    ulCloseDir( dir );
    return cnt;
}

static void remove_dir( const char *dir_name, void (*step)(void*,int), void *data ) {
    ulDir *dir = ulOpenDir( dir_name ) ;
    ulDirEnt *ent;
    while ( ent = ulReadDir( dir ) ) {
        if ( strcmp( ent->d_name, "." ) == 0 ) {
            // ignore "."
        } else if ( strcmp( ent->d_name, ".." ) == 0 ) {
            // ignore ".."
        } else if ( ent->d_isdir ) {
            SGPath child( dir_name );
            child.append( ent->d_name );
            remove_dir( child.c_str(), step, data );
        } else {
            SGPath child( dir_name );
            child.append( ent->d_name );
            unlink( child.c_str() );
            if (step) step( data, 1 );
        }
    }
    ulCloseDir( dir );
    rmdir( dir_name );
}


// remove selected files
void FGAdminUI::remove_selected() {
    char *f;

    install_b->deactivate();
    remove_b->deactivate();
    quit_b->deactivate();
    // traverse remove box and recursively remove each item
    for ( int i = 0; i <= remove_box->nitems(); ++i ) {
        if ( remove_box->checked( i ) ) {
            f = remove_box->text( i );
            SGPath dir( dest );
            dir.append( f );
            float old_max = progress->maximum();
            progress_label = "Removing ";
            progress_label += f;
            progress->label( progress_label.c_str() );
            progress->value( min_progress );
            main_window->cursor( FL_CURSOR_WAIT );
            progress->maximum( count_dir( dir.c_str() ) );
            remove_dir( dir.c_str(), &FGAdminUI::step, this );
            progress->value( min_progress );
            main_window->cursor( FL_CURSOR_DEFAULT );
            progress->label( "" );
            progress->maximum( old_max );
        }
    }
    quit_b->activate();
    install_b->activate();
    remove_b->activate();

    refresh_lists();
   
}

void FGAdminUI::step(void *data)
{
   Fl_Progress *p = ((FGAdminUI*)data)->progress;

   // we don't actually know the total work in advanced due to the
   // nature of tar archives, and it would be inefficient to prescan
   // the files, so just cycle the progress bar until we are done.
   float tmp = p->value() + 1;
   if ( tmp > max_progress ) {
       tmp = 0.0;
   }
   p->value( tmp );

   Fl::check();
}

void FGAdminUI::step(void *data, int n)
{
   Fl_Progress *p = ((FGAdminUI*)data)->progress;

   float tmp = p->value() + n;
   p->value( tmp );

   Fl::check();
}
