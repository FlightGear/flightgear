//----------------------------------------------------------------------------------------
// Name:        accounts.cc
// Purpose:     dialog box to manage accounts
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// GCC implementation
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma implementation "accounts.h"
#endif

//----------------------------------------------------------------------------------------
// Standard wxWindows headers
//----------------------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

//----------------------------------------------------------------------------------------
// Header of this .cpp file
//----------------------------------------------------------------------------------------

#include "accounts.h"  

//----------------------------------------------------------------------------------------
// Remaining headers
// ---------------------------------------------------------------------------------------

#include "app.h"
#include "frame.h"
#include "main.h"
#include "dial.h"

//----------------------------------------------------------------------------------------
// Event table: connect the events to the handler functions to process them
//----------------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(AccountsDialog, wxDialog)
    EVT_BUTTON(XRCID("AddAccountList"),     AccountsDialog::OnAddAccountList)
    EVT_BUTTON(XRCID("EditAccountList"),    AccountsDialog::OnAddAccountList)
    EVT_BUTTON(XRCID("RemoveAccountList"),  AccountsDialog::OnRemoveAccountList)
END_EVENT_TABLE()

//----------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------

AccountsDialog::AccountsDialog( wxWindow* parent )
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("Accounts"));

    //----Reach in for our controls-----------------------------------------------------
    AccountList  = XRCCTRL(*this, "AccountList", wxListCtrl);

    AccountList->InsertColumn(0, _("Name"),      wxLIST_FORMAT_LEFT, 100);
    AccountList->InsertColumn(1, _("Host"),      wxLIST_FORMAT_LEFT, 100);
    AccountList->InsertColumn(2, _("Username"),  wxLIST_FORMAT_LEFT, 100);

    Show();
}

void AccountsDialog::Show( void )
{
    wxConfig  *config = theApp::getConfig();
    wxString   str;
    long       dummy;
    bool       bCont;
    wxListItem item;
    long       i;

    //----Populate AccountList listctrl--------------------------------------------------
    config->SetPath(_T("/Accounts"));
    AccountList->DeleteAllItems();
    i = 0;
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
        AccountList->InsertItem(i, str);
        AccountList->SetItem(i, 1, config->Read(AccountList->GetItemText(i) +
                                                _T("/Host"), _T("")));
        AccountList->SetItem(i, 2, config->Read(AccountList->GetItemText(i) +
                                                _T("/Username"), _T("")));
        bCont = config->GetNextGroup(str, dummy);
        i++;
    }

    AccountList->SetColumnWidth(0, -1);
    AccountList->SetColumnWidth(1, -1);
    AccountList->SetColumnWidth(2, -1);
    if(AccountList->GetColumnWidth(0) < 100)        AccountList->SetColumnWidth(0,  100);
    if(AccountList->GetColumnWidth(1) < 100)        AccountList->SetColumnWidth(1,  100);
    if(AccountList->GetColumnWidth(2) < 100)        AccountList->SetColumnWidth(2,  100);
}

//----------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(AddAccountDialog, wxDialog)
    EVT_BUTTON(XRCID("Add"),            AddAccountDialog::OnAdd)
END_EVENT_TABLE()

AddAccountDialog::AddAccountDialog( wxWindow* parent, wxString Selection )
{
    wxConfig  *config = theApp::getConfig();
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("AddAccount"));

    //----Reach in for our controls-----------------------------------------------------
    Label        = XRCCTRL(*this, "Label",        wxStaticText);
    AccountName  = XRCCTRL(*this, "AccountName",  wxTextCtrl);
    HostName     = XRCCTRL(*this, "HostName",     wxTextCtrl);
    UserName     = XRCCTRL(*this, "UserName",     wxTextCtrl);
    Password     = XRCCTRL(*this, "Password",     wxTextCtrl);
    Confirm      = XRCCTRL(*this, "Confirm",      wxTextCtrl);

    if(!Selection.IsEmpty()) {
        SetTitle(_("Edit " + Selection));
        AccountName->SetValue(Selection);
        config->SetPath(_T("/Accounts/") + Selection);
        HostName->SetValue(config->Read(_T("Host"), _T("")));
        UserName->SetValue(config->Read(_T("Username"), _T("")));
        Password->SetValue(config->Read(_T("Password"), _T("")));
        Confirm->SetValue(config->Read(_T("Password"), _T("")));
    }
}

//----------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------

void AccountsDialog::OnAddAccountList(wxCommandEvent &event)
{
    long     sel = -1;
    wxString val;

    if(event.GetId() == XRCID("EditAccountList")) {
        if((sel = AccountList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0)
            val = AccountList->GetItemText(sel);
    }
    AddAccountDialog dialog(this, val);
    dialog.ShowModal();

    Show();
    wxGetApp().theFrame->ShowDirectoryControls();
}

void AccountsDialog::OnRemoveAccountList(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    long       sel = -1;
    int        isOK;

    if((sel=AccountList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0) {
        isOK = wxMessageBox(_("Really remove ") + AccountList->GetItemText(sel) + _T("?"),
                            _("Remove from Account List"),
                            wxOK|wxCANCEL|wxCENTRE);
        if(isOK == wxOK) {
            config->DeleteGroup(_T("/Accounts/") + AccountList->GetItemText(sel));
            AccountList->DeleteItem(sel);
        }
    }
    delete config;
    wxGetApp().theFrame->ShowDirectoryControls();
}

void AddAccountDialog::OnAdd(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();

    if(!Password->GetValue().IsSameAs(Confirm->GetValue())) {
        wxMessageBox(_("Try Again"),
                     _("Password Mismatch"),
                       wxICON_INFORMATION);
        return;
    }
    config->SetPath(_T("/Accounts/") + AccountName->GetValue());
    config->Write(_T("Host"),     HostName->GetValue());
    config->Write(_T("Username"), UserName->GetValue());
    config->Write(_T("Password"), Password->GetValue());
    delete config;

    if(wxGetApp().DefaultAccount.IsEmpty()) {
        wxGetApp().DefaultAccount = AccountName->GetValue();
    }

    // Well we wouldn't have added it if we didn't want to regiser
    // Thanks, AJ
    char user[256], pass[256], host[256];

    strncpy(user, UserName->GetValue().mb_str(*(wxGetApp().ConvIax)), sizeof(user));
    strncpy(pass, Password->GetValue().mb_str(*(wxGetApp().ConvIax)), sizeof(pass));
    strncpy(host, HostName->GetValue().mb_str(*(wxGetApp().ConvIax)), sizeof(host));
    iaxc_register(user, pass, host);


    AccountName->SetValue(_T(""));
    HostName->SetValue(_T(""));
    UserName->SetValue(_T(""));
    HostName->SetValue(_T(""));
    Confirm->SetValue(_T(""));
}

