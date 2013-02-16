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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#include <iostream>
#include <string>
#include <set>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#define unlink _unlink
#define mkdir _mkdir
#else // !_WIN32
#include <unistd.h>
#endif

#include <FL/Fl_File_Chooser.H>

#include <simgear/misc/sg_path.hxx>

#include "fgadmin.h"
#include "untarka.h"

using std::cout;
using std::endl;
using std::set;
using std::string;

extern string def_install_source;
extern string def_scenery_dest;

static const float min_progress = 0.0;
static const float max_progress = 5000.0;

// FIXME: Ugly hack to detect the situation below 
#ifdef FL_Volume_Down
//#if (FL_MAJOR_VERSION > 1)||((FL_MAJOR_VERSION == 1)&&(FL_MINOR_VERSION >= 3))
    // Fltk 1.3 or newer, need to use "fl_filename_free_list"
    #define FL_FREE_DIR_ENTRY(e) // do nothing, since "fl_filename_free_list" frees entire list
    #define FL_FREE_DIR_LIST(list,count) fl_filename_free_list(&list, count)
    #define FL_STAT(file,info) fl_stat( file.str().c_str(), info )
#else
    // Fltk < 1.3, "fl_filename_free_list", "fl_stat" not available
    #define FL_FREE_DIR_ENTRY(e) free(e)
    #define FL_FREE_DIR_LIST(list,count) free(list)
    #define FL_STAT(file,info) stat( file.str().c_str(), info );
#endif

/** Strip a single trailing '/' or '\\' */
static char* stripSlash(char* str)
{
    int l = strlen(str);
    if ((l>0)&&
        ((str[l-1]=='/')||(str[l-1]=='\\')))
    {
        str[l-1] = 0;
    }
    return str;
}

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

#if 0
// Like strcmp, but for strings
static int stringCompare(const void *string1, const void *string2)
{
    const string *s1 = (const string *)string1;
    const string *s2 = (const string *)string2;

    // Compare name first, and then index.
    return strcmp(s1->c_str(), s2->c_str());
}
#endif

void FGAdminUI::update_install_box() {
    set<string> file_list;

    install_box->clear();

    if ( source.length() ) {
        struct dirent **list;
        int nb = fl_filename_list( source.c_str(), &list );
        for ( int i = 0; i < nb; ++i ) {
            // find base name of archive file
            char base[FL_PATH_MAX];
            dirent *ent = list[i];
            stripSlash(ent->d_name);
            strncpy( base, ent->d_name, FL_PATH_MAX );
            const char *p = fl_filename_ext( base );
            int offset;
            string::size_type expected_length = 0;
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
                    file_list.insert( ent->d_name );
                } else {
                    // cout << install.str() << " exists." << endl;
                }
            }
            FL_FREE_DIR_ENTRY(ent);
        }
        FL_FREE_DIR_LIST(list, nb);

        for ( set<string>::iterator it = file_list.begin(); it != file_list.end(); ++it ) {
            install_box->add( it->c_str() );
        }

        install_box->redraw();
    }
}


// scan the source directory and update the install_box contents
void FGAdminUI::update_remove_box() {

    remove_box->clear();

    if ( dest.length() ) {
        string path[2];
        path[0] = dest + "/Terrain";
        path[1] = dest + "/Objects";
        if ( !fl_filename_isdir( path[0].c_str() ) ) {
            path[0] = dest;
            path[1] = "";
        } else if ( !fl_filename_isdir( path[1].c_str() ) ) {
            path[1] = "";
        }

        set<string> dir_list;
        for ( int i = 0; i < 2; i++ ) {
            if ( !path[i].empty() ) {
                dirent **list;
                int nb = fl_filename_list( path[i].c_str(), &list );
                for ( int i = 0; i < nb; ++i ) {
                    dirent *ent = list[i];
                    stripSlash(ent->d_name);
                    if ( strlen(ent->d_name) == 7 &&
                            ( ent->d_name[0] == 'e' || ent->d_name[0] == 'w' ) &&
                            ( ent->d_name[4] == 'n' || ent->d_name[4] == 's' ) ) {
                        dir_list.insert( ent->d_name );
                    }
                    FL_FREE_DIR_ENTRY(ent);
                }
                FL_FREE_DIR_LIST(list, nb);
            }
        }

        for ( set<string>::iterator it = dir_list.begin(); it != dir_list.end(); ++it ) {
            remove_box->add( it->c_str() );
        }

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
            struct ::stat info;
            FL_STAT( file, &info );
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


static unsigned long count_dir( const char *dir_name, bool top = true ) {
    unsigned long cnt = 0L;
    dirent **list;
    int nb = fl_filename_list( dir_name, &list );
    if ( nb != 0 ) {
        for ( int i = 0; i < nb; ++i ) {
            dirent *ent = list[i];
            stripSlash(ent->d_name);
            if ( strcmp( ent->d_name, "." ) == 0 ) {
                // ignore "."
            } else if ( strcmp( ent->d_name, ".." ) == 0 ) {
                // ignore ".."
            } else if ( fl_filename_isdir( ent->d_name ) ) {
                SGPath child( dir_name );
                child.append( ent->d_name );
                cnt += count_dir( child.c_str(), false );
            } else {
                cnt += 1;
            }
            FL_FREE_DIR_ENTRY(ent);
        }
        FL_FREE_DIR_LIST(list, nb);
    } else if ( top ) {
        string base = dir_name;
        size_t pos = base.rfind('/');
        string file = base.substr( pos );
        base.erase( pos );
        string path = base + "/Terrain" + file;
        cnt = count_dir( path.c_str(), false );
        path = base + "/Objects" + file;
        cnt += count_dir( path.c_str(), false );
    }
    return cnt;
}

static void remove_dir( const char *dir_name, void (*step)(void*,int), void *data, bool top = true ) {
    dirent **list;
    int nb = fl_filename_list( dir_name, &list );
    if ( nb > 0 ) {
        for ( int i = 0; i < nb; ++i ) {
            dirent *ent = list[i];
            SGPath child( dir_name );
            child.append( ent->d_name );
            stripSlash(ent->d_name);
            if ( strcmp( ent->d_name, "." ) == 0 ) {
                // ignore "."
            } else if ( strcmp( ent->d_name, ".." ) == 0 ) {
                // ignore ".."
            } else if ( child.isDir() ) {
                remove_dir( child.c_str(), step, data, false );
            } else {
                unlink( child.c_str() );
                if (step) step( data, 1 );
            }
            FL_FREE_DIR_ENTRY(ent);
        }
        FL_FREE_DIR_LIST(list, nb);
        rmdir( dir_name );
    } else if ( top ) {
        string base = dir_name;
        size_t pos = base.rfind('/');
        string file = base.substr( pos );
        base.erase( pos );
        string path = base + "/Terrain" + file;
        remove_dir( path.c_str(), step, data, false );
        path = base + "/Objects" + file;
        remove_dir( path.c_str(), step, data, false );
    }
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
