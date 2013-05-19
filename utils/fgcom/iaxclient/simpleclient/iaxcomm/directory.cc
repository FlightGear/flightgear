//----------------------------------------------------------------------------------------
// Name:        directory.cc
// Purpose:     dialog box to manage directory
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
    #pragma implementation "directory.h"
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

#include "directory.h"  

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
BEGIN_EVENT_TABLE(DirectoryDialog, wxDialog)
    EVT_BUTTON(XRCID("AddOTList"),               DirectoryDialog::OnAddOTList)
    EVT_BUTTON(XRCID("AddPhoneList"),            DirectoryDialog::OnAddPhoneList)
    EVT_BUTTON(XRCID("EditOTList"),              DirectoryDialog::OnAddOTList)
    EVT_BUTTON(XRCID("EditPhoneList"),           DirectoryDialog::OnAddPhoneList)
    EVT_BUTTON(XRCID("RemoveOTList"),            DirectoryDialog::OnRemoveOTList)
    EVT_BUTTON(XRCID("RemovePhoneList"),         DirectoryDialog::OnRemovePhoneList)
    EVT_BUTTON(XRCID("DialOTList"),              DirectoryDialog::OnDialOTList)
    EVT_BUTTON(XRCID("DialPhoneList"),           DirectoryDialog::OnDialPhoneList)

    EVT_LIST_ITEM_ACTIVATED(XRCID("OTList"),     DirectoryDialog::OnDialOTListActivatedEvent)
    EVT_LIST_ITEM_ACTIVATED(XRCID("PhoneList"),  DirectoryDialog::OnDialPhoneListActivatedEvent)
END_EVENT_TABLE()


//----------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------

DirectoryDialog::DirectoryDialog( wxWindow* parent )
{
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("Directory"));

    //----Reach in for our controls-----------------------------------------------------
    DirectoryNotebook  = XRCCTRL(*this, "DirectoryNotebook", wxNotebook);

    OTList       = XRCCTRL(*this, "OTList",      wxListCtrl);
    PhoneList    = XRCCTRL(*this, "PhoneList",   wxListCtrl);

    OTList->InsertColumn(     0, _("No"),        wxLIST_FORMAT_LEFT,  40);
    OTList->InsertColumn(     1, _("Name"),      wxLIST_FORMAT_LEFT, 100);
    OTList->InsertColumn(     2, _("Extension"), wxLIST_FORMAT_LEFT, 100);

    PhoneList->InsertColumn(  0, _("Name"),      wxLIST_FORMAT_LEFT, 100);
    PhoneList->InsertColumn(  1, _("Extension"), wxLIST_FORMAT_LEFT,  80);
    PhoneList->InsertColumn(  2, _("Ring Tone"), wxLIST_FORMAT_LEFT, 100);

    Show();
}

void DirectoryDialog::Show( void )
{
    wxConfig  *config = theApp::getConfig();
    wxString   str;
    long       dummy;
    bool       bCont;
    wxListItem item;
    long       i;

    //----Populate OTList listctrl--------------------------------------------------
    config->SetPath(_T("/OT"));
    i = 0;
    OTList->DeleteAllItems();
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
        OTList->InsertItem(i, str);
        OTList->SetItem(i, 1, config->Read(OTList->GetItemText(i) + _T("/Name"), _T("")));
        OTList->SetItem(i, 2, config->Read(OTList->GetItemText(i) + _T("/Extension"), _T("")));
        bCont = config->GetNextGroup(str, dummy);
        i++;
    }

    OTList->SetColumnWidth(0, -1);
    OTList->SetColumnWidth(1, -1);
    OTList->SetColumnWidth(2, -1);
    if(OTList->GetColumnWidth(0) <  40)        OTList->SetColumnWidth(0,   40);
    if(OTList->GetColumnWidth(1) < 100)        OTList->SetColumnWidth(1,  100);
    if(OTList->GetColumnWidth(2) < 100)        OTList->SetColumnWidth(2,  100);

    //----Populate PhoneList listctrl--------------------------------------------------
    config->SetPath(_T("/PhoneBook"));;
    PhoneList->DeleteAllItems();
    i=0;
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
        wxString ExtensionItem;
        wxString RingToneItem;

        PhoneList->InsertItem(i, str);

        ExtensionItem = config->Read(PhoneList->GetItemText(i) + _T("/Extension"), _T(""));
        RingToneItem  = config->Read(PhoneList->GetItemText(i) + _T("/RingTone"),_T(""));

        wxString ShortTone = RingToneItem.AfterLast(wxFILE_SEP_PATH);

        PhoneList->SetItem(i, 1, ExtensionItem);
        PhoneList->SetItem(i, 2, ShortTone);

        bCont = config->GetNextGroup(str, dummy);
        i++;
    }

    PhoneList->SetColumnWidth(0, -1);
    PhoneList->SetColumnWidth(1, -1);
    PhoneList->SetColumnWidth(2, -1);
    if(PhoneList->GetColumnWidth(0) < 100)     PhoneList->SetColumnWidth(0,  100);
    if(PhoneList->GetColumnWidth(1) < 100)     PhoneList->SetColumnWidth(1,  100);
    if(PhoneList->GetColumnWidth(2) < 100)     PhoneList->SetColumnWidth(2,  100);
}

//----------------------------------------------------------------------------------------


BEGIN_EVENT_TABLE(AddOTListDialog, wxDialog)
    EVT_BUTTON(XRCID("Add"),            AddOTListDialog::OnAdd)
END_EVENT_TABLE()

AddOTListDialog::AddOTListDialog( wxWindow* parent, wxString Selection )
{
    wxConfig  *config = theApp::getConfig();
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("AddOT"));

    //----Reach in for our controls-----------------------------------------------------
    OTNo         = XRCCTRL(*this, "OTNo",         wxSpinCtrl);
    Name         = XRCCTRL(*this, "Name",         wxTextCtrl);
    Extension    = XRCCTRL(*this, "Extension",    wxTextCtrl);

    if(!Selection.IsEmpty()) {
        SetTitle(_("Edit " + Selection));
        OTNo->SetValue(Selection);
        config->SetPath(_T("/OT/") + Selection);
        Name->SetValue(config->Read(_T("Name"), _T("")));
        Extension->SetValue(config->Read(_T("Extension"), _T("")));
    }
}

//----------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(AddPhoneListDialog, wxDialog)
    EVT_BUTTON(XRCID("BrowseRingTone"),  AddPhoneListDialog::OnBrowse)
    EVT_BUTTON(XRCID("PreviewRingTone"), AddPhoneListDialog::OnPreviewRingTone)

    EVT_BUTTON(XRCID("Add"),             AddPhoneListDialog::OnAdd)
END_EVENT_TABLE()

AddPhoneListDialog::AddPhoneListDialog( wxWindow* parent, wxString Selection )
{
    wxConfig  *config = theApp::getConfig();
    wxXmlResource::Get()->LoadDialog(this, parent, wxT("AddPhoneList"));

    //----Reach in for our controls-----------------------------------------------------
    Name         = XRCCTRL(*this, "Name",         wxTextCtrl);
    Extension    = XRCCTRL(*this, "Extension",    wxTextCtrl);
    RingTone     = XRCCTRL(*this, "RingTone",     wxTextCtrl);

    if(!Selection.IsEmpty()) {
        SetTitle(_("Edit " + Selection));
        Name->SetValue(Selection);
        config->SetPath(_T("/PhoneBook/") + Selection);
        Extension->SetValue(config->Read(_T("Extension"), _T("")));
        RingTone->SetValue(config->Read(_T("RingTone"), _T("")));
    }
}

//----------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------

void DirectoryDialog::OnAddOTList(wxCommandEvent &event)
{
    long     sel = -1;
    wxString val;

    if(event.GetId() == XRCID("EditOTList")) {
        if((sel = OTList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0)
            val = OTList->GetItemText(sel);
    }
    AddOTListDialog dialog(this, val);
    dialog.ShowModal();

    Show();
    wxGetApp().theFrame->ShowDirectoryControls();
}

void DirectoryDialog::OnAddPhoneList(wxCommandEvent &event)
{
    long     sel = -1;
    wxString val;

    if(event.GetId() == XRCID("EditPhoneList")) {
        if((sel = PhoneList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0)
            val = PhoneList->GetItemText(sel);
    }
    AddPhoneListDialog dialog(this, val);
    dialog.ShowModal();

    Show();
}

//----------------------------------------------------------------------------------------

void DirectoryDialog::OnDialOTListActivatedEvent(wxListEvent &event)
{
    OnDialOTList(*((wxCommandEvent *) &event));
}

void DirectoryDialog::OnDialOTList(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    long       sel = -1;
    wxString   DialString;

    sel=OTList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
    if(sel >= 0) {
        DialString = config->Read(_T("/OT/") + OTList->GetItemText(sel) + _T("/Extension"),
                                  _T(""));

        // A DialString in quotes means look up name in phone book
        if(DialString.StartsWith(_T("\""))) {
            DialString = DialString.Mid(1, DialString.Len() -2);
            DialString = config->Read(_T("/PhoneBook/") + DialString + _T("/Extension"), _T(""));
        }
        Dial(DialString);
        Close(TRUE);
    }
}

void DirectoryDialog::OnDialPhoneListActivatedEvent(wxListEvent &event)
{
    OnDialPhoneList(*((wxCommandEvent *) &event));
}

void DirectoryDialog::OnDialPhoneList(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    long       sel = -1;
    wxString   DialString;

    sel=PhoneList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
    if(sel >= 0) {
        DialString = config->Read(_T("/PhoneBook/") + PhoneList->GetItemText(sel) + _T("/Extension"),
                                  _T(""));
        Dial(DialString);
        Close(TRUE);
    }
}

//----------------------------------------------------------------------------------------

void DirectoryDialog::OnRemoveOTList(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    long       sel = -1;
    int        isOK;

    if((sel=OTList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0) {
        isOK = wxMessageBox(_T("Really remove One Touch ") + OTList->GetItemText(sel) + _T("?"),
                            _T("Remove from One Touch List"), wxOK|wxCANCEL|wxCENTRE);
        if(isOK == wxOK) {
            config->DeleteGroup(_T("/OT/") + OTList->GetItemText(sel));
            OTList->DeleteItem(sel);
        }
    }
    delete config;
    wxGetApp().theFrame->ShowDirectoryControls();
}

void DirectoryDialog::OnRemovePhoneList(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    long       sel = -1;
    int        isOK;

    if((sel=PhoneList->GetNextItem(sel,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) >= 0) {
        isOK = wxMessageBox(_T("Really remove ") + PhoneList->GetItemText(sel) + _T("?"),
                            _T("Remove from Phone Book"), wxOK|wxCANCEL|wxCENTRE);
        if(isOK == wxOK) {
            config->DeleteGroup(_T("/PhoneBook/") + PhoneList->GetItemText(sel));
            PhoneList->DeleteItem(sel);
        }
    }
    delete config;
}

//----------------------------------------------------------------------------------------

void AddOTListDialog::OnAdd(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();
    wxString   Path;

    Path.Printf(_T("/OT/%d"), OTNo->GetValue());
    config->SetPath(Path);
    config->Write(_T("Name"),      Name->GetValue());
    config->Write(_T("Extension"), Extension->GetValue());
    delete config;

    Name->SetValue(_T(""));
    Extension->SetValue(_T(""));
}

//----------------------------------------------------------------------------------------

void AddPhoneListDialog::OnAdd(wxCommandEvent &event)
{
    wxConfig  *config = theApp::getConfig();

    config->SetPath(_T("/PhoneBook/") + Name->GetValue());
    config->Write(_T("Extension"), Extension->GetValue());
    config->Write(_T("RingTone"),  RingTone->GetValue());
    delete config;

    Name->SetValue(_T(""));
    Extension->SetValue(_T(""));
    RingTone->SetValue(_T(""));
}

void AddPhoneListDialog::OnPreviewRingTone(wxCommandEvent &event)
{
    // We only want to preview a single ring
    wxGetApp().CallerIDRing.LoadTone(RingTone->GetValue(), 0);
    wxGetApp().CallerIDRing.Start(1);
    wxGetApp().CallerIDRing.LoadTone(RingTone->GetValue(), 10);
}

void AddPhoneListDialog::OnBrowse(wxCommandEvent &event)
{
    wxString dirHome;
    wxGetHomeDir(&dirHome);

    wxFileDialog where(NULL, _("Raw sound file"), dirHome, _T(""), _T("*.*"), wxOPEN );
    where.ShowModal();

    RingTone->SetValue(where.GetPath());
}
