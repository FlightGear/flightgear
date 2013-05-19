//----------------------------------------------------------------------------------------
// Name:        app.h
// Purpose:     Core application includes
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _APP_H_
#define _APP_H_

//----------------------------------------------------------------------------------------
// Shared defines
//----------------------------------------------------------------------------------------
#define MAX_CALLS 2

//----------------------------------------------------------------------------------------
// Shared headers
//----------------------------------------------------------------------------------------

#include "wx/app.h"
#include "wx/checkbox.h"
#include "wx/choice.h"
#include "wx/combobox.h"
#include "wx/config.h"
#include "wx/dialog.h"
#include "wx/frame.h"
#include "wx/gauge.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/snglinst.h"
#include "wx/spinctrl.h"
#include "wx/taskbar.h"
#include "wx/textctrl.h"
#include "wx/xrc/xmlres.h"

#include "iaxclient.h"

#ifdef __WXMSW__
#include <wx/msw/winundef.h> // needed for xmlres
#endif

/* for the silly key state stuff :( */
#ifdef __WXGTK__
#include <gdk/gdk.h>
#endif

#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif



#endif  // _APP_H_
