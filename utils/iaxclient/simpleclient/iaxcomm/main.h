//----------------------------------------------------------------------------------------
// Name:        main.h
// Purpose:     Main module includes
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _MAIN_H_
#define _MAIN_H_

//----------------------------------------------------------------------------------------
// GCC interface
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma interface "main.h"
#endif

//----------------------------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------------------------

#include "app.h"
#include "frame.h"
#include "calls.h"
#include "ringer.h"

enum
{
    IAXCLIENT_EVENT = 500
};

void RegisterByName(wxString RegName);

  #ifdef __WXMSW__
//----------------------------------------------------------------------------------------
// Class definition: MyTaskBarIcon
//----------------------------------------------------------------------------------------

class MyTaskBarIcon : public wxTaskBarIcon
{
public:
    MyTaskBarIcon() {};

    void          OnRestore(wxCommandEvent&);
    void          OnIconize(wxIconizeEvent&);
    void          OnHide(wxCommandEvent&);
    void          OnExit(wxCommandEvent&);

    virtual void  OnLButtonDown(wxEvent&);
    virtual void  OnLButtonDClick(wxEvent&);
    virtual void  OnRButtonDown(wxEvent&);

DECLARE_EVENT_TABLE()
};
#endif

//----------------------------------------------------------------------------------------
// Class definition: theApp
//----------------------------------------------------------------------------------------

class theApp : public wxApp
{

public:

    virtual bool  OnInit(); 
    virtual int   OnExit();

    MyFrame      *theFrame;

    Ringer        IncomingRing;
    Ringer        RingbackTone;
    Ringer        IntercomTone;
    Ringer        CallerIDRing;

    wxString      IncomingRingName;
    wxString      RingBackToneName;
    wxString      IntercomToneName;
    wxString      CallerIDRingName;

    wxString      InputDevice;
    wxString      OutputDevice;
    wxString      SpkInputDevice;
    wxString      SpkOutputDevice;
    wxString      RingDevice;

    int           nCalls;
    wxString      Name;
    wxString      Number;
    wxString      DefaultAccount;

    wxMBConv      *ConvIax;

    static wxConfig *getConfig();

  #ifdef __WXMSW__
    MyTaskBarIcon *theTaskBarIcon;
  #endif

protected:

private:

    void          RegisterByName(wxString RegName);
    
                  // Helper function. loads the specified XRC XML resource file.
    void          load_xrc_resource( const wxString& xrc_filename );   

                             //! The single instance checker
    wxSingleInstanceChecker *m_single_instance_checker;
};

//----------------------------------------------------------------------------------------
// wxWindows macro: Declare the application
//----------------------------------------------------------------------------------------

DECLARE_APP( theApp );

#endif  // _MAIN_H_
