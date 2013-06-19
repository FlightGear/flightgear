//----------------------------------------------------------------------------------------
// Name:        frame,h
// Purpose:     Describes main dialog
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _FRAME_H_
#define _FRAME_H_

//----------------------------------------------------------------------------------------
// GCC interface
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma interface "frame.h"
#endif

//----------------------------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------------------------

#include "app.h"
#include "calls.h"
#include "directory.h"
#include "accounts.h"
#include "ringer.h"
#include "wx/menu.h"
#include "wx/html/helpctrl.h"

#define LEVEL_MAX                   (0)
#define LEVEL_MIN                   (-50)
#define DEFAULT_SILENCE_THRESHOLD   1 // positive is "auto"

//----------------------------------------------------------------------------------------
// Class definition: MyTimer
//----------------------------------------------------------------------------------------

class MyTimer : public wxTimer
{

public:

    void        Notify();
};

//----------------------------------------------------------------------------------------
// Class definition: MyFrame
//----------------------------------------------------------------------------------------

class MyFrame : public wxFrame
{

public:
    
    MyFrame( wxWindow* parent=(wxWindow *)NULL);
   ~MyFrame();
   
    void        ShowDirectoryControls();
    void        OnSize(wxSizeEvent &event);
    void        OnShow();
    void        OnNotify();
    void        OnHangup(wxCommandEvent &event);
    void        OnHoldKey(wxCommandEvent &event);
    void        OnSpeakerKey(wxCommandEvent &event);
    void        OnQuit(wxEvent &event);
    void        OnPTTChange(wxCommandEvent &event);
    void        OnSilenceChange(wxCommandEvent &event);
    void        OnInputSlider(wxScrollEvent &event);
    void        OnOutputSlider(wxScrollEvent &event);
    bool        GetPTTState();
    void        CheckPTT();
    void        SetPTT(bool state);
    void        RePanel(wxString Name);
    void        ApplyCodecs(void);
    void        ApplyFilters(void);

    // Handlers for library-initiated events
    void        HandleEvent(wxCommandEvent &evt);
    int         HandleIAXEvent(iaxc_event *e);
    int         HandleStatusEvent(char *msg);
    int         HandleLevelEvent(float input, float output);

    wxGauge    *Input;
    wxGauge    *Output;
    wxSlider   *InputSlider;
    wxSlider   *OutputSlider;
    wxChoice   *Account;
    wxComboBox *Extension;
    CallList   *Calls;

    wxHtmlHelpController* help;

    wxString    IntercomPass;

    bool        RingOnSpeaker;
    bool        UsingSpeaker;
    bool        AGC;
    bool        AAGC;
    bool        CN;
    bool        NoiseReduce;
    bool        EchoCancel;

    bool        AllowuLawVal;
    bool        AllowaLawVal;
    bool        AllowGSMVal;
    bool        AllowSpeexVal;
    bool        AllowiLBCVal;

    int         PreferredBitmap;

    bool        SPXTuneVal;
    bool        SPXEnhanceVal;
    int         SPXQualityVal;
    int         SPXBitrateVal;
    int         SPXABRVal;
    bool        SPXVBRVal;
    int         SPXComplexityVal;
    
private:

    // An icon for the corner of dialog and application's taskbar button
    wxIcon      m_icon;
    wxPanel    *aPanel;
    int         MessageTicks;

    void        OnPrefs(wxCommandEvent& event);
    void        OnAccounts(wxCommandEvent& event);
    void        OnDevices(wxCommandEvent& event);
    void        OnDirectory(wxCommandEvent& event);
    void        OnExit(wxCommandEvent& event);
    void        OnAbout(wxCommandEvent& event);
    void        OnHelp(wxCommandEvent& event);
    void        OnOneTouch(wxCommandEvent& event);
    void        OnKeyPad(wxCommandEvent& event);
    void        OnDialDirect(wxCommandEvent& event);
    void        OnTransfer(wxCommandEvent& event);
    void        AddPanel(wxWindow *parent, wxString Name);
    void        OnAccountChoice(wxCommandEvent &event);

#ifdef __WXGTK__
    GdkWindow *keyStateWindow;
#endif

    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------------------
// End single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#endif  // _FRAME_H_
