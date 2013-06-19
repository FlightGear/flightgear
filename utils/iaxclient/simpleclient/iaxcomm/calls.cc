//----------------------------------------------------------------------------------------
// Name:        calls.cc
// Purpose:     Call appearances listctrl
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
    #pragma implementation "calls.h"
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

#include "calls.h"  

//----------------------------------------------------------------------------------------
// Remaining headers
// ---------------------------------------------------------------------------------------

#include "app.h"
#include "main.h"
#include "prefs.h"
#include "frame.h"
#include "devices.h"
#include <wx/ffile.h>
#include <wx/textdlg.h>

//----------------------------------------------------------------------------------------
// Event table: connect the events to the handler functions to process them
//----------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CallList, wxListCtrl)
    EVT_SIZE                 (                CallList::OnSize)  
    EVT_LIST_ITEM_SELECTED   (XRCID("Calls"), CallList::OnSelect)
    EVT_LIST_ITEM_RIGHT_CLICK(XRCID("Calls"), CallList::OnRClick)
    EVT_LIST_ITEM_ACTIVATED  (XRCID("Calls"), CallList::OnDClick)
END_EVENT_TABLE()

//----------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------

CallList::CallList(wxWindow *parent, int nCalls, wxWindowID id, const wxPoint& pos, 
                    const wxSize& size, long style)
                  : wxListCtrl( parent, id, pos, size, style)
{
    wxConfig   *config = theApp::getConfig();
    long        i;
    wxListItem  item;

    m_parent = parent;

    config->SetPath(_T("/Prefs"));

    // Column Headings
    InsertColumn( 0, _(""),       wxLIST_FORMAT_CENTER, (20));
    InsertColumn( 1, _("State"),  wxLIST_FORMAT_CENTER, (60));
    InsertColumn( 2, _("Remote"), wxLIST_FORMAT_LEFT,  (200));

    Hide();

#if !defined(__UNICODE__)
    wxFont font   = GetFont();
    font.SetPointSize(11);
    font.SetFamily(wxSWISS);
    SetFont(font);
#endif

    for(i=0;i<nCalls;i++) {
        InsertItem(i,wxString::Format(_T("%ld"), i + 1), 0);
        SetItem(i, 2, _T(""));
        item.m_itemId=i;
        item.m_mask = 0;
        SetItem(item);
    }

    SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

    Refresh();
    Show();
    AutoSize();
}

void CallList::AutoSize()
{
    SetColumnWidth(2, GetClientSize().x - 85);
}

void CallList::OnSize(wxSizeEvent &event)
{
    event.Skip();
#ifdef __WXMSW__
    // XXX FIXME: for some reason not yet investigated, this crashes Linux-GTK (for SK, at least).
    // XXX2 This causes a crash _later_ in MacOSX -- but when compiled
    // with a debugging wx library, it causes an immediate assertion
    // failure saying column 2 is out of range.  Maybe it happens before
    // the columns are added or something.  Dunno.  But, the resize
    // later when you select a call is OK.
    AutoSize();
#endif
}

void CallList::OnSelect(wxListEvent &event)
{
    int selected = event.m_itemIndex;
    iaxc_unquelch(selected);
    iaxc_select_call(selected);
}

void CallList::OnDClick(wxListEvent &event)
{
    //Don't need to select, because single click should have done it
    iaxc_dump_call();
}

void CallList::OnRClick(wxListEvent &event)
{
    //Transfer
    int      selected = event.m_itemIndex;
    char     ext[256];
    wxString Title;
  
    Title.Printf(_T("Transfer Call %d"), selected);
    wxTextEntryDialog dialog(this,
                             _T("Target Extension:"),
                             Title,
                             _T(""),
                             wxOK | wxCANCEL);

    if(dialog.ShowModal() != wxID_CANCEL) {

        strncpy(ext, dialog.GetValue().mb_str(*(wxGetApp().ConvIax)), sizeof(ext));

        iaxc_blind_transfer_call(selected, ext);
    }
}

wxString CallList::GetCodec(struct iaxc_ev_call_state c)
{
	switch (c.format) {
	case (1 << 0):	return(_T("G.723.1"));	/* G.723.1 compression */
	case (1 << 1):	return(_T("GSM"));	/* GSM compression */
	case (1 << 2):	return(_T("u-law"));	/* Raw mu-law data (G.711) */
	case (1 << 3):	return(_T("a-law"));	/* Raw A-law data (G.711) */
	case (1 << 4):	return(_T("G726"));	/* ADPCM, 32kbps  */
	case (1 << 5):	return(_T("ADPCM"));	/* ADPCM IMA */
	case (1 << 6):	return(_T("SLINEAR"));	/* Raw 16-bit Signed Linear (8000 Hz) PCM */
	case (1 << 7):	return(_T("LPC10"));	/* LPC10, 180 samples/frame */
	case (1 << 8):	return(_T("G.729a"));	/* G.729a Audio */
	case (1 << 9):	return(_T("Speex"));	/* Speex Audio */
	case (1 <<10):	return(_T("iLBC"));	/* iLBC Audio */
	default:	return(_T(""));
	}
}

int CallList::HandleStateEvent(struct iaxc_ev_call_state c)
{
    wxConfig  *config = theApp::getConfig();
    wxString   str;
    long       dummy;
    bool       bCont;
    static int selectedcall = -1;
    wxString   ws;

    if(c.state & IAXC_CALL_STATE_RINGING) {
      wxGetApp().theFrame->Show();
      wxGetApp().theFrame->Raise();
    }

    int i;
    int nCalls = wxGetApp().nCalls;
    for(i=0; i<nCalls; i++)
        SetItem(i, 1, _T(""));

    // first, handle inactive calls
    if(!(c.state & IAXC_CALL_STATE_ACTIVE)) {
        SetItem(c.callNo, 2, _T(""));
        SetItem(c.callNo, 1, _T(""));
        wxGetApp().RingbackTone.Stop();
        wxGetApp().IncomingRing.Stop();
        wxGetApp().CallerIDRing.Stop();
    } else {
        bool     outgoing = c.state & IAXC_CALL_STATE_OUTGOING;
        bool     ringing  = c.state & IAXC_CALL_STATE_RINGING;
        bool     complete = c.state & IAXC_CALL_STATE_COMPLETE;
        bool     selected = c.state & IAXC_CALL_STATE_SELECTED;

        wxString Info;
        wxString Codec;
        wxString RemoteName;
        wxString Remote;
        

        RemoteName = wxString(c.remote_name, *(wxGetApp().ConvIax));
        Info  = RemoteName.AfterLast('@');	// Hide username:password
        Info  = Info.BeforeFirst('/');          // Remove extension in outbound call
        					// (it will be duplicated in <>)

        Remote = wxString(c.remote, *(wxGetApp().ConvIax));
        if(!Remote.IsEmpty())			// Additional info in Remote
            Info += _T(" <") + Remote + _T(">");

        Codec = GetCodec(c);			// Negotiated codec
        if(!Codec.IsEmpty())
	    Info += _T(" [") + GetCodec(c) + _T("]");	// Indicate Negotiated codec

        SetItem(c.callNo, 2, Info );

        if(selected)
            selectedcall = c.callNo;

        if(outgoing) {
            if(ringing) {
                SetItem(c.callNo, 1, _T("ring out"));
                wxGetApp().RingbackTone.Start(0);
            } else {
                wxGetApp().RingbackTone.Stop();
                if(complete) {
                    // I really need to clean up this spaghetti code
                    if(selected)
                        SetItem(c.callNo, 1, _T("ACTIVE"));
                    else
                        if(c.callNo == selectedcall)
                            SetItem(c.callNo, 1, _T(""));
                        else
                            SetItem(c.callNo, 1, _T("ACTIVE"));

                    wxGetApp().RingbackTone.Stop();
                } else {
                    // not accepted yet..
                    SetItem(c.callNo, 1, _T("---"));
                }
            }
        } else {
            if(ringing) {
                SetItem(c.callNo, 1, _T("ring in"));

                // Look for the caller in our phonebook
                config->SetPath(_T("/PhoneBook"));
                bCont = config->GetFirstGroup(str, dummy);
                ws = wxString(c.remote_name, *(wxGetApp().ConvIax));
                while ( bCont ) {
                    if(str.IsSameAs(ws))
                        break;
                    bCont = config->GetNextGroup(str, dummy);
                }

                if(!str.IsSameAs(ws)) {
                    // Add to phone book if not there already
                    str.Printf(_T("%s/Extension"), wxString(c.remote_name, *(wxGetApp().ConvIax)).c_str());
                    config->Write(str, c.remote);
                } else {
                    // Since they're in the phone book, look for ringtone
                    str.Printf(_T("%s/RingTone"), wxString(c.remote_name, *(wxGetApp().ConvIax)).c_str());
                    wxGetApp().CallerIDRingName = config->Read(str, _T(""));
                }

                if(strcmp(c.local_context, "intercom") == 0) {
                    if(config->Read(_T("/Prefs/IntercomPass"), _T("s")).IsSameAs(wxString(c.local, *(wxGetApp().ConvIax)))) {
                        wxGetApp().IntercomTone.Start(1);
                        iaxc_millisleep(1000);
                        iaxc_unquelch(c.callNo);
                        iaxc_select_call(c.callNo);

                        wxGetApp().theFrame->UsingSpeaker = true;
                        SetAudioDevices(wxGetApp().SpkInputDevice,
                                        wxGetApp().SpkOutputDevice,
                                        wxGetApp().RingDevice);
                    }
                } else {
                    if(wxGetApp().CallerIDRingName.IsEmpty()) {
                        wxGetApp().IncomingRing.Start(1);
                    } else {
                        wxGetApp().CallerIDRing.LoadTone(wxGetApp().CallerIDRingName, 10);
                        wxGetApp().CallerIDRing.Start(1);
                    }
                }
            } else {
                wxGetApp().IncomingRing.Stop();
                wxGetApp().CallerIDRing.Stop();
                if(complete) {
                    SetItem(c.callNo, 1, _T("ACTIVE"));
                } else { 
                    // not accepted yet..  shouldn't happen!
                    SetItem(c.callNo, 1, _T("???"));
                }
            }
        } 
    }
    
    if((c.state & IAXC_CALL_STATE_SELECTED)) 
        SetItemState(c.callNo,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
    else
        SetItemState(c.callNo,~wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);

    AutoSize();
    Refresh();

    return 0;
}
