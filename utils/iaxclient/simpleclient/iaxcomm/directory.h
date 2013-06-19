//----------------------------------------------------------------------------------------
// Name:        directory.h
// Purpose:     Describes directory dialog
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

//----------------------------------------------------------------------------------------
// GCC interface
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma interface "directory.h"
#endif

//----------------------------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------------------------

#include "app.h"

class DirectoryDialog : public wxDialog
{
public: 
    DirectoryDialog( wxWindow* parent );

    wxListCtrl  *OTList;
    wxListCtrl  *PhoneList;
    wxListCtrl  *AccountList;
        
private:

    void         Show(void);

    wxNotebook  *DirectoryNotebook;
    void         OnAddOTList(wxCommandEvent &event);
    void         OnRemoveOTList(wxCommandEvent &event);
    void         OnDialOTList(wxCommandEvent &event);
    void         OnDialOTListActivatedEvent(wxListEvent &event);
    void         OnAddPhoneList(wxCommandEvent &event);
    void         OnRemovePhoneList(wxCommandEvent &event);
    void         OnDialPhoneList(wxCommandEvent &event);
    void         OnDialPhoneListActivatedEvent(wxListEvent &event);

    DECLARE_EVENT_TABLE()

};

class AddOTListDialog : public wxDialog
{
public: 
    AddOTListDialog( wxWindow* parent, wxString selection );

    wxSpinCtrl  *OTNo;
    wxTextCtrl  *Name;
    wxTextCtrl  *Extension;
        
private:

    void         OnAdd(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

};

class AddPhoneListDialog : public wxDialog
{
public: 
    AddPhoneListDialog( wxWindow* parent, wxString selection );

    wxTextCtrl  *Name;
    wxTextCtrl  *Extension;
    wxTextCtrl  *RingTone;
        
private:

    void         OnAdd(wxCommandEvent &event);
    void         OnBrowse(wxCommandEvent &event);
    void         OnPreviewRingTone(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

};

#endif  //_DIRECTORY_H_

