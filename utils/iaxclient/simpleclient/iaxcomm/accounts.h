//----------------------------------------------------------------------------------------
// Name:        accounts.h
// Purpose:     Describes accounts dialog
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

//----------------------------------------------------------------------------------------
// GCC interface
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma interface "accounts.h"
#endif

//----------------------------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------------------------

#include "app.h"

class AccountsDialog : public wxDialog
{
public: 
    AccountsDialog( wxWindow* parent );

    wxListCtrl  *AccountList;
        
private:

    void         Show(void);

    void         OnAddAccountList(wxCommandEvent &event);
    void         OnRemoveAccountList(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

};

class AddAccountDialog : public wxDialog
{
public: 
    AddAccountDialog( wxWindow* parent, wxString selection );

    wxStaticText *Label;
    wxTextCtrl   *AccountName;
    wxTextCtrl   *HostName;
    wxTextCtrl   *UserName;
    wxTextCtrl   *Password;
    wxTextCtrl   *Confirm;
        
private:

    void         OnAdd(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

};

#endif  //_ACCOUNTS_H_

