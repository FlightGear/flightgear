//----------------------------------------------------------------------------------------
// Name:        dial.cc
// Purpose:     dial class
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
    #pragma implementation "dial.h"
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

#include "dial.h"  

//----------------------------------------------------------------------------------------
// Remaining headers
// ---------------------------------------------------------------------------------------

#include "app.h"
#include "frame.h"
#include "main.h"
#include "wx/tokenzr.h"

void DialEntry(wxString& EntryName)
{
    wxString Msg;
    wxString FQIN;

    wxConfig *config = theApp::getConfig();
    wxString  val;
    wxString  KeyPath;


    KeyPath = _T("/Entries/") + EntryName;
    config->SetPath(KeyPath);

    if(!config->Exists(KeyPath)) {
        EntryName << _T(" unknown");
        wxMessageBox(KeyPath, EntryName);
        return;
    }
    wxString AccountName = config->Read(_T("Account"), _T(""));
    wxString Extension   = config->Read(_T("Extension"), _T("s"));

    Dial(AccountName + _T("/") + Extension);
}

void Dial( wxString DialStr )
{
    wxConfig *config = theApp::getConfig();
    wxString  FQIN;

    wxString  AccountInfo = DialStr.BeforeLast('/');    // Empty   if no '/'
    wxString  Extension   = DialStr.AfterLast('/');     // dialstr if no '/'

    if(DialStr.IsEmpty())
        return;

    if(AccountInfo.IsEmpty()) {
        AccountInfo = wxGetApp().DefaultAccount;

        // Dialstr has no "/" and no default server: add default extension
        if(AccountInfo.IsEmpty()) {
            AccountInfo = Extension;
            Extension  = _T("s");
        }
    }

    wxString  RegInfo    = AccountInfo.BeforeLast('@'); // Empty   if no '@'
    wxString  Host       = AccountInfo.AfterLast('@');

    wxString  Username   = RegInfo.BeforeFirst(':');
    wxString  Password   = RegInfo.AfterFirst(':');     // Empty if no ':'


    if(RegInfo.IsEmpty()) {
        config->SetPath(_T("/Accounts/") + Host);
        Host     = config->Read(_T("Host"), Host);
        Username = config->Read(_T("Username"), _T(""));
        Password = config->Read(_T("Password"), _T(""));
    }

    FQIN.Printf(_T("%s:%s@%s/%s"),
                Username.c_str(),
                Password.c_str(),
                Host.c_str(),
                Extension.c_str());

    iaxc_call((char *)(const char *)FQIN.mb_str(*(wxGetApp().ConvIax)));
}

