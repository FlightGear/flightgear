// For compilers that supports precompilation , includes  wx/wx.h  
#include  "wx/wxprec.h"  

#ifndef WX_PRECOMP 
#include  "wx/wx.h"
#endif 

#include "wx/cmdline.h"
#include "wx/listctrl.h"
#include "wx/combobox.h"
#include "wx/tokenzr.h"
#include "wx/stattext.h"
#include "wx/string.h"
#include "iaxclient.h"


/* for the silly key state stuff :( */
#ifdef __WXGTK__
#include <gdk/gdk.h>
#endif

#ifdef __WXMAC__
#include <Carbon/Carbon.h>
#endif

#define LEVEL_MAX -10
#define LEVEL_MIN -50
#define DEFAULT_SILENCE_THRESHOLD 1 // positive is "auto"

#ifndef DEFAULT_NUM_CALLS
#define DEFAULT_NUM_CALLS 4
#endif

#ifndef DEFAULT_DESTSERV
#define DEFAULT_DESTSERV "guest@misery.digium.com"
#endif

#ifndef DEFAULT_DESTEXT  
#define DEFAULT_DESTEXT  "s@default"
#endif

class IAXClient : public wxApp
{
          public:
	          virtual bool OnInit();
		  virtual bool OnCmdLineParsed(wxCmdLineParser& p);
		  virtual void OnInitCmdLine(wxCmdLineParser& p);

		  bool optNoDialPad;
		  wxString optDestination;
		  long optNumCalls;
		  wxString optRegistration;
};

DECLARE_APP(IAXClient)

IMPLEMENT_APP(IAXClient) 

// forward decls for callbacks
extern "C" {
     static int iaxc_callback(iaxc_event e);
     int doTestCall(int ac, char **av);
}



// event constants
enum
{
    ID_DTMF_1 = 0,
    ID_DTMF_2,
    ID_DTMF_3,
    ID_DTMF_4,
    ID_DTMF_5,
    ID_DTMF_6,
    ID_DTMF_7,
    ID_DTMF_8,
    ID_DTMF_9,
    ID_DTMF_STAR,
    ID_DTMF_O,
    ID_DTMF_POUND,

    ID_DIAL = 100,
    ID_HANG,

    ID_QUIT = 200,

    ID_PTT = 300,
    ID_MUTE,
    ID_SILENCE,
    ID_REGISTER,

    ID_CALLS = 400,
    
    IAXCLIENT_EVENT = 500,

    ID_MAX
};


static char *buttonlabels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#" };



class IAXTimer : public wxTimer
{
  public:
    void IAXTimer::Notify(); 
};


class IAXCalls : public wxListCtrl
{
    public:
      IAXCalls(wxWindow *parent, int nCalls);
      void IAXCalls::OnSelect(wxListEvent &evt);
      int IAXCalls::HandleStateEvent(struct iaxc_ev_call_state c);
      void IAXCalls::OnSize(wxSizeEvent& evt);
      void IAXCalls::AutoSize(); 
      int IAXCalls::GetTotalHeight();
      int nCalls;

protected:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(IAXCalls, wxListCtrl)
	EVT_LIST_ITEM_SELECTED(ID_CALLS, IAXCalls::OnSelect)
	EVT_SIZE(IAXCalls::OnSize)
END_EVENT_TABLE()

IAXCalls::IAXCalls(wxWindow *parent, int inCalls) 
  : wxListCtrl(parent, ID_CALLS, 
      wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL, 
      wxDefaultValidator, _T("calls")), 
    nCalls(inCalls)
{
  // initialze IAXCalls control    
  long i;

  wxListItem item;
  item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
  item.m_image = -1;
  item.m_text = _T("Call");
  item.SetTextColour(*wxLIGHT_GREY);
  item.SetBackgroundColour(*wxWHITE);
  InsertColumn(0, item);
  item.m_text = _T("State");
  InsertColumn(1, item);
  item.m_text = _T("Remote");
  InsertColumn(2, item);

  Hide();
  for(i=0;i<nCalls;i++) {
      InsertItem(i,wxString::Format("%ld", i+1), 0);
      SetItem(i, 2, _T("No call"));
      item.m_itemId=i;
      item.m_mask = 0;
      item.SetTextColour(*wxLIGHT_GREY);
      item.SetBackgroundColour(*wxWHITE);
      SetItem(item);
  }
  Refresh();
  Show();
  AutoSize();
}

int IAXCalls::GetTotalHeight() {
    // Return height to be what we need
#if 0  
    // this doesn't work when called very early; the main area size
  // isn't set yet.  So, we'll just guess, and see how this work on
  // platforms.
    int x,y, y2;
    ((wxScrolledWindow *)m_mainWin)->GetVirtualSize(&x, &y);
    y2 = ((wxWindow *)m_headerWin)->GetSize().y;
    fprintf(stderr, "GetTotalHeight: y=%d, y2=%d\n", y, y2);
    return y+y2;
#endif
    // GTK actual: 19*nCalls + 23
    return 20*nCalls + 25 + 15;  
}

void IAXCalls::AutoSize() {
    // Set column widths.
    int width = 0;

    // first columns are auto-sized
    for(int i=0;i<=1;i++) {
#if defined(__WXMSW__)
      // wxLIST_AUTOSIZE makes it too small for headings
      SetColumnWidth(i,35+i*15);
#else
      SetColumnWidth(i,wxLIST_AUTOSIZE);
#endif
      width+=GetColumnWidth(i);
    }

    // last column is at least the remainder in width.
    // minus some padding
    int min=GetClientSize().x - width - 5;
    SetColumnWidth(2,wxLIST_AUTOSIZE);
    if(GetColumnWidth(2) < min)
      SetColumnWidth(2,min);
}

void IAXCalls::OnSize(wxSizeEvent& evt) {
    evt.Skip();
    AutoSize();
}


void IAXCalls::OnSelect(wxListEvent &evt) {
	int selected = evt.m_itemIndex; //GetSelection();
	iaxc_select_call(selected);
}


class IAXFrame : public wxFrame
{
public: 
    IAXFrame(const wxChar *title, int xpos, int ypos, int width, int height);

    ~IAXFrame();

    void IAXFrame::OnDTMF(wxEvent &evt);
    void IAXFrame::OnDial(wxEvent &evt);
    void IAXFrame::OnHangup(wxEvent &evt);
    void IAXFrame::OnQuit(wxEvent &evt);
    void IAXFrame::OnPTTChange(wxCommandEvent &evt);
    void IAXFrame::OnSilenceChange(wxCommandEvent &evt);
    void IAXFrame::OnNotify(void);
    void IAXFrame::OnRegisterMenu(wxCommandEvent &evt);

    // utility method
    void IAXFrame::RegisterFromString(wxString value);

    // Handlers for library-initiated events
    void IAXFrame::HandleEvent(wxCommandEvent &evt);
    int IAXFrame::HandleIAXEvent(iaxc_event *e);
    int IAXFrame::HandleStatusEvent(char *msg);
    int IAXFrame::HandleLevelEvent(float input, float output);

    bool IAXFrame::GetPTTState();
    void IAXFrame::CheckPTT();
    void IAXFrame::SetPTT(bool state);

    wxGauge *input; 
    wxGauge *output; 
    IAXTimer *timer;
    wxStaticText *iaxServLabel;
    wxComboBox *iaxServ;
    wxStaticText *iaxDestLabel;
    wxComboBox *iaxDest;
    wxStaticText *muteState;

    IAXCalls *calls;

    bool pttMode;  // are we in PTT mode?
    bool pttState; // is the PTT button pressed?
    bool silenceMode;  // are we in silence suppression mode?


protected:
    DECLARE_EVENT_TABLE()

#ifdef __WXGTK__
    GdkWindow *keyStateWindow;
#endif
};

static IAXFrame *theFrame;

int IAXCalls::HandleStateEvent(struct iaxc_ev_call_state c)
{
    wxListItem stateItem; // for all the state color

    stateItem.m_itemId = c.callNo;

    if(c.state & IAXC_CALL_STATE_RINGING) {
      theFrame->Raise();
    }

    // first, handle inactive calls
    if(!(c.state & IAXC_CALL_STATE_ACTIVE)) {
	//fprintf(stderr, "state for item %d is free\n", c.callNo);
	SetItem(c.callNo, 2, _T("No call") );
	SetItem(c.callNo, 1, _T("") );
	stateItem.SetTextColour(*wxLIGHT_GREY);
	stateItem.SetBackgroundColour(*wxWHITE);
    } else {
	// set remote 
	SetItem(c.callNo, 2, c.remote );

	
	bool outgoing = c.state & IAXC_CALL_STATE_OUTGOING;
	bool ringing = c.state & IAXC_CALL_STATE_RINGING;
	bool complete = c.state & IAXC_CALL_STATE_COMPLETE;

	if( ringing && !outgoing ) {
	    stateItem.SetTextColour(*wxBLACK);
	    stateItem.SetBackgroundColour(*wxRED);
	} else {
	    stateItem.SetTextColour(*wxBLUE);
	    stateItem.SetBackgroundColour(*wxWHITE);
	}

	if(outgoing) {
	  if(ringing) 
	    SetItem(c.callNo, 1, _T("r") );
	  else if(complete)
	    SetItem(c.callNo, 1, _T("o") );
	  else // not accepted yet..
	    SetItem(c.callNo, 1, _T("c") );
	} else {
	  if(ringing) 
	    SetItem(c.callNo, 1, _T("R") );
	  else if(complete)
	    SetItem(c.callNo, 1, _T("I") );
	  else // not accepted yet..  shouldn't happen!
	    SetItem(c.callNo, 1, _T("C") );
	} 
	// XXX do something more noticable if it's incoming, ringing!
    }
   
    SetItem( stateItem ); 
    
    // select if necessary
    if((c.state & IAXC_CALL_STATE_SELECTED) &&
	!(GetItemState(c.callNo,wxLIST_STATE_SELECTED|wxLIST_STATE_SELECTED))) 
    {
	//fprintf(stderr, "setting call %d to selected\n", c.callNo);
	SetItemState(c.callNo,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
    }
    AutoSize();
    Refresh();

    return 0;
}

void IAXFrame::OnNotify()
{
    if(pttMode) CheckPTT(); 
}

void IAXTimer::Notify()
{
    theFrame->OnNotify();
}


IAXFrame::IAXFrame(const wxChar *title, int xpos, int ypos, int width, int height)
  : wxFrame((wxFrame *) NULL, -1, title, wxPoint(xpos, ypos), wxSize(width, height)), calls(NULL)
{
    wxBoxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);
    wxPanel *aPanel = new wxPanel(this);
    wxButton *dialButton;

    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *row3sizer = new wxBoxSizer(wxHORIZONTAL);
    pttMode = false;

    /* add status bar first; otherwise, sizer doesn't take it into
     * account */
    CreateStatusBar();


    /* Set up Menus */
    /* NOTE: Mac doesn't use a File/Exit item, and Application/Quit is
	automatically added for us */
 
    wxMenu *fileMenu = new wxMenu();
    fileMenu->Append(ID_REGISTER, _T("&Register\tCtrl-R"));

// Mac: no quit item (it goes in app menu)
// Win: "exit" item.
// Linux and others: Quit.
#if !defined(__WXMAC__)
#if defined(__WXMSW__)
    fileMenu->Append(ID_QUIT, _T("E&xit\tCtrl-X"));
#else
    fileMenu->Append(ID_QUIT, _T("&Quit\tCtrl-Q"));
#endif
#endif

    wxMenu *optionsMenu = new wxMenu();
    optionsMenu->AppendCheckItem(ID_PTT, _T("Enable &Push to Talk\tCtrl-P"));
    optionsMenu->AppendCheckItem(ID_SILENCE, _T("Enable &Silence Suppression\tCtrl-S"));
   

    wxMenuBar *menuBar = new wxMenuBar();

    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(optionsMenu, _T("&Options"));

    SetMenuBar(menuBar);

    wxBoxSizer *row1sizer; 
    if(wxGetApp().optNoDialPad) {
	row1sizer = new wxBoxSizer(wxVERTICAL);

	/* volume meters */
	row1sizer->Add(input  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, wxDefaultPosition, wxSize(15,15), 
	      wxGA_HORIZONTAL,  wxDefaultValidator, _T("input level")),1, wxEXPAND); 

	row1sizer->Add(output  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, wxDefaultPosition, wxSize(15,15), 
	      wxGA_HORIZONTAL,  wxDefaultValidator, _T("output level")),1, wxEXPAND); 
    } else {
	row1sizer = new wxBoxSizer(wxHORIZONTAL);
	/* DialPad Buttons */
	wxGridSizer *dialpadsizer = new wxGridSizer(3);
	for(int i=0; i<12;i++)
	{
	    dialpadsizer->Add(
	      new wxButton(aPanel, i, wxString(buttonlabels[i]),
		      wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT), 
		1, wxEXPAND|wxALL, 3);
	}
	row1sizer->Add(dialpadsizer,1, wxEXPAND);

	/* volume meters */
	row1sizer->Add(input  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, wxDefaultPosition, wxSize(15,50), 
	      wxGA_VERTICAL,  wxDefaultValidator, _T("input level")),0, wxEXPAND); 

	row1sizer->Add(output  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, wxDefaultPosition, wxSize(15,50), 
	      wxGA_VERTICAL,  wxDefaultValidator, _T("output level")),0, wxEXPAND); 
    }

    topsizer->Add(row1sizer,1,wxEXPAND);

    // Add call appearances
    if(wxGetApp().optNumCalls > 1) {
      int nCalls = wxGetApp().optNumCalls;

      // XXX need to fix sizes better.
      topsizer->Add(calls = new IAXCalls(aPanel, nCalls), 0, wxEXPAND);
      topsizer->SetItemMinSize(calls, -1, calls->GetTotalHeight()+5);
    }

    /* Server */
    topsizer->Add(iaxServLabel = new wxStaticText(aPanel, -1, _T("Server:")));
    topsizer->Add(iaxServ = new wxComboBox(aPanel, -1, _T(DEFAULT_DESTSERV), 
	wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    iaxServ->Append(DEFAULT_DESTSERV);
    iaxServ->Append("guest@ast1");
    iaxServ->Append("guest@asterisk");

    /* Destination */
    topsizer->Add(iaxDestLabel = new wxStaticText(aPanel, -1, _T("Number:")));
    topsizer->Add(iaxDest = new wxComboBox(aPanel, -1, _T(DEFAULT_DESTEXT), 
	wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    iaxDest->Append(DEFAULT_DESTEXT);
    iaxDest->Append("8068");
    iaxDest->Append("208");
    iaxDest->Append("600");

    /* main control buttons */    
    row3sizer->Add(dialButton = new wxButton(aPanel, ID_DIAL, _T("Dial"),
	    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),1, wxEXPAND|wxALL, 3);

    row3sizer->Add(new wxButton(aPanel, ID_HANG, _T("Hang Up"),
	    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),1, wxEXPAND|wxALL, 3);

    /* make dial the default button */
    dialButton->SetDefault();
#if 0
    row3sizer->Add(quitButton = new wxButton(aPanel, 100, wxString("Quit"),
	    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),1, wxEXPAND|wxALL, 3);
#endif

    topsizer->Add(row3sizer,0,wxEXPAND);

    topsizer->Add(muteState = new wxStaticText(aPanel,-1,"PTT Disabled",
	    wxDefaultPosition, wxDefaultSize),0,wxEXPAND);


    aPanel->SetSizer(topsizer);
    topsizer->SetSizeHints(aPanel);

    panelSizer->Add(aPanel,1,wxEXPAND);
    SetSizer(panelSizer);	
    panelSizer->SetSizeHints(this);


#ifdef __WXGTK__
    // window used for getting keyboard state
    GdkWindowAttr attr;
    keyStateWindow = gdk_window_new(NULL, &attr, 0);
#endif

    timer = new IAXTimer();
    timer->Start(100);
    //output = new wxGauge(this, -1, 100); 
}

bool IAXFrame::GetPTTState()
{
    bool pressed;
#ifdef __WXMAC__
    KeyMap theKeys;
    GetKeys(theKeys);
    // that's the Control Key (by experimentation!)
    pressed = theKeys[1] & 0x08;
    //fprintf(stderr, "%p %p %p %p\n", theKeys[0], theKeys[1], theKeys[2], theKeys[3]);
#else
#ifdef __WXMSW__
    pressed = GetAsyncKeyState(VK_CONTROL)&0x8000;
#else
    int x, y;
    GdkModifierType modifiers;
    gdk_window_get_pointer(keyStateWindow, &x, &y, &modifiers);
    pressed = modifiers & GDK_CONTROL_MASK;
#endif
#endif
    return pressed;
}

void IAXFrame::SetPTT(bool state)
{
	pttState = state;
	if(pttState) {
		iaxc_set_silence_threshold(-99); //unmute input
		iaxc_set_audio_output(1);  // mute output
	} else {
		iaxc_set_silence_threshold(0);  // mute input
		iaxc_set_audio_output(0);  // unmute output
	}

        muteState->SetLabel( pttState ? "Talk" : "Mute");
}

void IAXFrame::CheckPTT()
{
    bool newState = GetPTTState();
    if(newState == pttState) return;
 
    SetPTT(newState);
}

void IAXFrame::OnDTMF(wxEvent &evt)
{
	iaxc_send_dtmf(*buttonlabels[evt.m_id]);
}

void IAXFrame::OnDial(wxEvent &evt)
{
	wxChar Destination[128];

	wxStrcpy(Destination, theFrame->iaxServ->GetValue());
        wxStrcat(Destination, _T("/"));
	wxStrcat(Destination, theFrame->iaxDest->GetValue());

	iaxc_call(Destination);
}

void IAXFrame::OnHangup(wxEvent &evt)
{
	iaxc_dump_call();
}

void IAXFrame::OnQuit(wxEvent &evt)
{
	Close(TRUE);
}

void IAXFrame::RegisterFromString(wxString value) {
	wxStringTokenizer tok(value, _T(":@"));
	char user[256], pass[256], host[256];

	if(tok.CountTokens() != 3) {
	    theFrame->SetStatusText("error in registration format");
	    return;
	}

	strncpy( user , tok.GetNextToken().c_str(), 256);
	strncpy( pass , tok.GetNextToken().c_str(), 256);
	strncpy( host , tok.GetNextToken().c_str(), 256);

	theFrame->iaxServ->Append(value);
	theFrame->iaxServ->SetValue(value);

	//fprintf(stderr, "Registering user %s pass %s host %s\n", user, pass, host);
	iaxc_register(user, pass, host);
}


void IAXFrame::OnRegisterMenu(wxCommandEvent &evt) {
	wxTextEntryDialog dialog(this,
	    _T("Register with a remote asterisk server"),
	    _T("Format is user:password@hostname"),
	    _T(wxGetApp().optRegistration),
	    wxOK | wxCANCEL);

	if(dialog.ShowModal() == wxID_OK)
	    RegisterFromString(dialog.GetValue());
}

void IAXFrame::OnPTTChange(wxCommandEvent &evt)
{
	pttMode = evt.IsChecked();
	
	if(pttMode) {
		SetPTT(GetPTTState());	
	} else {
		SetPTT(true);	
		if(silenceMode) {
			iaxc_set_silence_threshold(DEFAULT_SILENCE_THRESHOLD);
		} else {
			iaxc_set_silence_threshold(-99);
		}
		iaxc_set_audio_output(0);  // unmute output
		muteState->SetLabel("PTT Disabled");
	}
}

void IAXFrame::OnSilenceChange(wxCommandEvent &evt)
{
	// XXX get the actual state!
	silenceMode =  evt.IsChecked();
	
	if(pttMode) return;

	if(silenceMode) {
		iaxc_set_silence_threshold(DEFAULT_SILENCE_THRESHOLD);
	} else {
		iaxc_set_silence_threshold(-99);
	}
}


int IAXFrame::HandleStatusEvent(char *msg)
{
    theFrame->SetStatusText(msg);
  return 1;
}

int IAXFrame::HandleLevelEvent(float input, float output)
{
  int inputLevel, outputLevel;

  if (input < LEVEL_MIN)
    input = LEVEL_MIN; 
  else if (input > LEVEL_MAX)
    input = LEVEL_MAX;
  inputLevel = (int)input - (LEVEL_MIN); 

  if (output < LEVEL_MIN)
    output = LEVEL_MIN; 
  else if (input > LEVEL_MAX)
    output = LEVEL_MAX;
  outputLevel = (int)output - (LEVEL_MIN); 

  static int lastInputLevel = 0;
  static int lastOutputLevel = 0;


    if(lastInputLevel != inputLevel) {
      theFrame->input->SetValue(inputLevel); 
      lastInputLevel = inputLevel;
    }

    if(lastOutputLevel != outputLevel) {
      theFrame->output->SetValue(outputLevel); 
      lastOutputLevel = outputLevel;
    }

 return 1;
}


int IAXFrame::HandleIAXEvent(iaxc_event *e)
{
    int ret = 0;
    switch(e->type) {
	case IAXC_EVENT_LEVELS:
	    ret = HandleLevelEvent(e->ev.levels.input, e->ev.levels.output);
	    break;
	case IAXC_EVENT_TEXT:
	    ret = HandleStatusEvent(e->ev.text.message);
	    break;
	case IAXC_EVENT_STATE:
	    if(calls)
	      ret = theFrame->calls->HandleStateEvent(e->ev.call);
	    break;
	default:
	    break;  // not handled
    }
    return ret;
}

void IAXFrame::HandleEvent(wxCommandEvent &evt)
{
    iaxc_event *e = (iaxc_event *)evt.GetClientData();
    //fprintf(stderr, "In HandleEvent\n");
    HandleIAXEvent(e);
    free (e);
}


BEGIN_EVENT_TABLE(IAXFrame, wxFrame)
	EVT_BUTTON(0,IAXFrame::OnDTMF)
	EVT_BUTTON(1,IAXFrame::OnDTMF)
	EVT_BUTTON(2,IAXFrame::OnDTMF)
	EVT_BUTTON(3,IAXFrame::OnDTMF)
	EVT_BUTTON(4,IAXFrame::OnDTMF)
	EVT_BUTTON(5,IAXFrame::OnDTMF)
	EVT_BUTTON(6,IAXFrame::OnDTMF)
	EVT_BUTTON(7,IAXFrame::OnDTMF)
	EVT_BUTTON(8,IAXFrame::OnDTMF)
	EVT_BUTTON(9,IAXFrame::OnDTMF)
	EVT_BUTTON(10,IAXFrame::OnDTMF)
	EVT_BUTTON(11,IAXFrame::OnDTMF)
	EVT_BUTTON(ID_DIAL,IAXFrame::OnDial)
	EVT_BUTTON(ID_HANG,IAXFrame::OnHangup)

	EVT_MENU(ID_PTT,IAXFrame::OnPTTChange)
	EVT_MENU(ID_SILENCE,IAXFrame::OnSilenceChange)
	EVT_MENU(ID_REGISTER,IAXFrame::OnRegisterMenu)
	EVT_MENU(ID_QUIT,IAXFrame::OnQuit)

	EVT_MENU(IAXCLIENT_EVENT, IAXFrame::HandleEvent)
END_EVENT_TABLE()


IAXFrame::~IAXFrame()
{
    iaxc_dump_all_calls();
    for(int i=0;i<10;i++) {
      iaxc_millisleep(100);
    }
    iaxc_stop_processing_thread();
    exit(0);	
    iaxc_shutdown();
}


void IAXClient::OnInitCmdLine(wxCmdLineParser& p)
{
     // declare our CmdLine options and stuff.
     p.AddSwitch(_T("d"),_T("disable-dialpad"),_T("Disable Dial Pad"));
     p.AddOption(_T("n"),_T("calls"),_T("number of call appearances"),
	 wxCMD_LINE_VAL_NUMBER,wxCMD_LINE_PARAM_OPTIONAL);
     p.AddOption(_T("r"),_T("registration"),_T("Registration"),
	 wxCMD_LINE_VAL_STRING,wxCMD_LINE_PARAM_OPTIONAL);
     p.AddParam(_T("destination"),wxCMD_LINE_VAL_STRING,wxCMD_LINE_PARAM_OPTIONAL);

}

bool IAXClient::OnCmdLineParsed(wxCmdLineParser& p)
{
    if(p.Found(_T("d"))) { 
	optNoDialPad = true;
	//fprintf(stderr, "-d option found\n");
    }
    if(p.Found(_T("n"), &optNumCalls)) {
	//fprintf(stderr, "got nCalls (%d)", optNumCalls);
    }
    if(p.Found(_T("r"), &optRegistration)) {
	//fprintf(stderr, "got Registration (%s)", optRegistration.c_str());
    }
    if(p.GetParamCount() >= 1) {
	optDestination=p.GetParam(0);
	//fprintf(stderr, "dest is %s\n", optDestination.c_str());
    }

    return true;
}

bool IAXClient::OnInit() 
{ 
	optNoDialPad = false;
	optNumCalls = DEFAULT_NUM_CALLS;

	if(!wxApp::OnInit())
	  return false;

	theFrame = new IAXFrame("IAXTest", 0,0,130,220);


	theFrame->Show(TRUE); 
	SetTopWindow(theFrame); 
   
        iaxc_initialize(AUDIO_INTERNAL_PA, wxGetApp().optNumCalls);

	//        iaxc_set_encode_format(IAXC_FORMAT_GSM);
        iaxc_set_silence_threshold(-99);
	iaxc_set_event_callback(iaxc_callback);

        iaxc_start_processing_thread();

	if(!optRegistration.IsEmpty()) {
	    theFrame->RegisterFromString(optRegistration);
	}
    
	if(!optDestination.IsEmpty()) 
	    iaxc_call((char *)optDestination.c_str());
    

	return true; 
}



extern "C" {

   /* yes, using member variables, and polling for changes
    * isn't really a nice way to do callbacks.  BUT, I've tried the
    * "right" ways, unsuccessfully:
    *
    * 1) I've tried using wxGuiMutexEnter/Leave, but that deadlocks (and it should, I guess),
    * because sometimes these _are_ called in response to GUI events (i.e. the status callback eventually
    * gets called when you press hang up.
    *
    * 2) I've tried using events and ::wxPostEvent, but that also deadlocks (at least on GTK),
    * because it calls wxWakeUpIdle, which calls wxGuiMutexEnter, which deadlocks also, for some reason.
    *
    * So, this isn't ideal, but it works, until I can figure out a better way
    */

   // handle events via posting.
#ifdef USE_GUILOCK 
   static int iaxc_callback(iaxc_event e)
   {
      int ret;
      if (!wxThread::IsMain()) wxMutexGuiEnter();
	ret = theFrame->HandleIAXEvent(&e);
      if (!wxThread::IsMain()) wxMutexGuiLeave();
      return ret;
   }
#else
   static int iaxc_callback(iaxc_event e)
   {
      iaxc_event *copy;
      wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, IAXCLIENT_EVENT);

      copy = (iaxc_event *)malloc(sizeof(iaxc_event));
      memcpy(copy,&e,sizeof(iaxc_event));

      evt.SetClientData(copy);

      //fprintf(stderr, "Posting Event\n");
      wxPostEvent(theFrame, evt);

      // pretend we handle all events.
      return 1;
   }
#endif

}
