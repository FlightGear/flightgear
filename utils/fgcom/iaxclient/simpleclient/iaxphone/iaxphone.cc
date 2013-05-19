// For compilers that supports precompilation , includes  wx/wx.h include
#include "wx/wxprec.h"

#ifndef WX_PRECOMP 
#include  "wx/wx.h"
#endif 

#include "wx/cmdline.h"
#include "wx/listctrl.h"
#include "wx/combobox.h"
#include "wx/tokenzr.h"
#include "wx/stattext.h"
#include "wx/string.h"
#include "wx/config.h"
#include "wx/confbase.h"
#include "iaxclient.h"
#include "iaxphone.h"

static IAXFrame *theFrame;

RegList::RegList(wxWindow *parent, int regcount) 
                 : wxListCtrl(parent, ID_REG, 
                   wxDefaultPosition, wxDefaultSize,
                   wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES, 
                   wxDefaultValidator, _T("registrations")), 
                   regcount(regcount)
{
    wxListItem item;
    item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
    item.m_image = -1;
    item.m_text = _T("Server");
    item.SetTextColour(*wxLIGHT_GREY);
    item.SetBackgroundColour(*wxWHITE);
    InsertColumn(0, item);
    item.m_text = _T("State");
    InsertColumn(1, item);

    Show();
    AutoSize();
}

void RegList::AutoSize() 
{
    int min = GetClientSize().x - 50;

    SetColumnWidth(0, min);
    SetColumnWidth(1, 45);
}

void RegList::OnSize(wxSizeEvent& evt) {
    evt.Skip();
    AutoSize();
}

void RegList::OnSelect(wxListEvent &evt) 
{
    //  Toggle Registration Status
}

IAXCalls::IAXCalls(wxWindow *parent, int inCalls) 
                 : wxListCtrl(parent, ID_CALLS, 
                   wxDefaultPosition, wxDefaultSize,
                   wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_HRULES, 
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

int IAXCalls::GetTotalHeight() 
{
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

void IAXCalls::AutoSize() 
{
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

void IAXCalls::OnSelect(wxListEvent &evt) 
{
    int selected = evt.m_itemIndex; //GetSelection();
    iaxc_select_call(selected);
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
                 : wxFrame((wxFrame *) NULL, -1, title, wxPoint(xpos, ypos),
                   wxSize(width, height))
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString str;
    long     dummy;
    bool     bCont;

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
    optionsMenu->Append(ID_AUDIO,  "&Audio ...", "Show audio settings dialog");
    optionsMenu->Append(ID_SERVER, "&Servers ...", "Manage servers list");
    optionsMenu->Append(ID_SPEED,  "&Speed Dials ...", "Manage Speed Dials");
   
    wxMenuBar *menuBar = new wxMenuBar();

    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(optionsMenu, _T("&Options"));

    SetMenuBar(menuBar);

    wxBoxSizer *row1sizer; 
    if(wxGetApp().optNoDialPad) {
        row1sizer = new wxBoxSizer(wxVERTICAL);

        /* volume meters */
        row1sizer->Add(input  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, 
                                            wxDefaultPosition, wxSize(15,15),
                                            wxGA_HORIZONTAL,  wxDefaultValidator, 
                                            _T("input level")),1, wxEXPAND); 

        row1sizer->Add(output  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN, 
                                             wxDefaultPosition, wxSize(15,15), 
                                             wxGA_HORIZONTAL,  wxDefaultValidator,
                                             _T("output level")),1, wxEXPAND); 
    } else {
        row1sizer = new wxBoxSizer(wxHORIZONTAL);
        /* DialPad Buttons */
        wxGridSizer *dialpadsizer = new wxGridSizer(3);
        for(int i=0; i<12;i++)
        {
            dialpadsizer->Add(new wxButton(aPanel, i, wxString(buttonlabels[i]),
                                           wxDefaultPosition, wxDefaultSize,
                                           wxBU_EXACTFIT), 1, wxEXPAND|wxALL, 3);
        }
        row1sizer->Add(dialpadsizer,1, wxEXPAND);

        /* volume meters */
        row1sizer->Add(input  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN,
                                            wxDefaultPosition, wxSize(15,50), 
                                            wxGA_VERTICAL,  wxDefaultValidator, 
                                            _T("input level")),0, wxEXPAND); 

        row1sizer->Add(output  = new wxGauge(aPanel, -1, LEVEL_MAX-LEVEL_MIN,
                                             wxDefaultPosition, wxSize(15,50), 
                                             wxGA_VERTICAL,  wxDefaultValidator,
                                             _T("output level")),0, wxEXPAND); 
    }

    topsizer->Add(row1sizer,1,wxEXPAND);

    // Add call registrations
    topsizer->Add(registrations = new RegList(aPanel, 2), 0, wxEXPAND);
    topsizer->SetItemMinSize(registrations, -1, 85);

    // Add call appearances
    if(wxGetApp().optNumCalls > 1) {
        int nCalls = wxGetApp().optNumCalls;

        // XXX need to fix sizes better.
        topsizer->Add(calls = new IAXCalls(aPanel, nCalls), 0, wxEXPAND);
        topsizer->SetItemMinSize(calls, -1, calls->GetTotalHeight()+5);
    }

    /* Speed Dial */
    topsizer->Add(SpeedDial = new wxComboBox(aPanel, ID_SPEEDDIAL, "", 
                                      wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    config->SetPath("/Speed Dials");
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
      SpeedDial->Append(str);
      bCont = config->GetNextGroup(str, dummy);
    }

    /* main control buttons */    
    row3sizer->Add(dialButton = new wxButton(aPanel, ID_DIAL, _T("Dial"),
                                             wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),
                                             1, wxEXPAND|wxALL, 3);

    row3sizer->Add(new wxButton(aPanel, ID_HANG, _T("Hang Up"),
                                wxDefaultPosition, wxDefaultSize,wxBU_EXACTFIT),
                                1, wxEXPAND|wxALL, 3);

    /* make dial the default button */
    dialButton->SetDefault();
  #if 0
    row3sizer->Add(quitButton = new wxButton(aPanel, 100, wxString("Quit"),
                                             wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),
                                             1, wxEXPAND|wxALL, 3);
  #endif

    topsizer->Add(row3sizer,0,wxEXPAND);

    topsizer->Add(muteState = new wxStaticText(aPanel,-1,"PTT Disabled",
                                               wxDefaultPosition, wxDefaultSize),
                                               0,wxEXPAND);

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
    theFrame->DialBySpeedDialName(SpeedDial->GetValue());
}

void IAXFrame::OnHangup(wxEvent &evt)
{
    iaxc_dump_call();
}

void IAXFrame::OnQuit(wxEvent &evt)
{
    Close(TRUE);
}

void IAXFrame::RegisterByName(wxString RegName) {
    wxConfig    *config = new wxConfig("iaxPhone");
    wxChar      KeyPath[256];
    wxListItem  item;
    long        index;

    wxStringTokenizer tok(RegName, _T(":@"));
    char user[256], pass[256], host[256];

    if(strlen(RegName) == 0)
        return;

    if(tok.CountTokens() == 3) {

        strncpy( user , tok.GetNextToken().c_str(), 256);
        strncpy( pass , tok.GetNextToken().c_str(), 256);
        strncpy( host , tok.GetNextToken().c_str(), 256);
    } else {
        // Check if it's a Speed Dial
        wxStrcpy(KeyPath,     "/Servers/");
        wxStrcat(KeyPath,     RegName);
        config->SetPath(KeyPath);
        if(!config->Exists(KeyPath)) {
            theFrame->SetStatusText("Register format error");
            return;
        }
        wxStrcpy(user, config->Read("Username", ""));
        wxStrcpy(pass, config->Read("Password", ""));
        wxStrcpy(host, config->Read("Host", ""));
    }
    iaxc_register(user, pass, host);

    if(registrations->FindItem(-1, RegName) < 0)
    {
        registrations->Hide();

        item.SetText(RegName);
        item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
        item.m_image = -1;
        item.SetTextColour(*wxLIGHT_GREY);
        item.SetBackgroundColour(*wxWHITE);
        index = registrations->InsertItem(item);
        registrations->SetItem(index, 0, RegName);

        // iaxc_register is type void
        // need to find some way to update status -- is there any
        // server data in the "reg accepted" message??

        registrations->SetItem(index, 1, "----");

        registrations->Refresh();
        registrations->Show();
        registrations->AutoSize();
    }
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

void IAXFrame::OnAudioDialog(wxCommandEvent& WXUNUSED(event))
{
    AudioDialog dialog(this, "Audio Properties", wxDefaultPosition, wxSize(520,220));
    dialog.ShowModal();
}

void IAXFrame::OnServerDialog(wxCommandEvent& WXUNUSED(event))
{
    ServerDialog dialog(this, "Server Dialog", wxDefaultPosition, wxSize(500,260));
    dialog.ShowModal();
}

void IAXFrame::OnDialDialog(wxCommandEvent& WXUNUSED(event))
{
    DialDialog dialog(this, "Dial Dialog", wxDefaultPosition, wxSize(500,240));
    dialog.ShowModal();
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

void IAXFrame::DialBySpeedDialName(wxString name)
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString KeyPath;
    wxString ServerName;
    wxString Extension;
    wxString Destination;

    if(name.IsEmpty())
        return;

    KeyPath = "/Speed Dials/" + name;

    if(config->Exists(KeyPath)) {
        config->SetPath(KeyPath);

        ServerName = config->Read("Server",    "");
        Extension =  config->Read("Extension", "");

        KeyPath = "/Servers/" + ServerName;
        config->SetPath(KeyPath);

        Destination  = config->Read("Username", "") + ":" +
                       config->Read("Password", "") + "@" +
                       config->Read("Host", "")     + "/" + 
                       Extension;
    } else {
        Destination = name;
    }
    iaxc_call((char *)Destination.c_str());
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
    EVT_MENU(ID_QUIT,IAXFrame::OnQuit)
    EVT_MENU(ID_AUDIO,IAXFrame::OnAudioDialog)
    EVT_MENU(ID_SERVER,IAXFrame::OnServerDialog)
    EVT_MENU(ID_SPEED,IAXFrame::OnDialDialog)

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
    wxConfig *config = new wxConfig("iaxPhone");

    wxString str;
    wxString reginfo;
    long     dummy;
    bool     bCont;

    if(!wxApp::OnInit())
        return false;

    optNoDialPad = (config->Read("/DialPad",  1l) == 0);
    optNumCalls  =  config->Read("/NumCalls", 4l);

    theFrame = new IAXFrame("IAXPhone", 0,0,150,220);

    theFrame->Show(TRUE); 
    SetTopWindow(theFrame); 

    iaxc_initialize(AUDIO_INTERNAL_PA, wxGetApp().optNumCalls);
   
    theFrame->SetAudioDeviceByName(config->Read("/Input Device", ""),
                                   config->Read("/Output Device", ""),
                                   config->Read("/Ring Device", ""));

    //    iaxc_set_encode_format(IAXC_FORMAT_GSM);
    iaxc_set_silence_threshold(-99);
    iaxc_set_event_callback(iaxc_callback);

    iaxc_start_processing_thread();

    // Register from wxConfig

    config->SetPath("/Servers");
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
        reginfo = str + "/Auto Register";
        if((config->Read(reginfo, 0l) != 0)) {
            theFrame->RegisterByName(str);
        }
        bCont = config->GetNextGroup(str, dummy);
    }

    if(!optRegistration.IsEmpty()) {
        theFrame->RegisterByName(optRegistration);
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

AudioDialog::AudioDialog(wxWindow *parent, const wxString& title,
                         const wxPoint& pos, const wxSize& size, const long WXUNUSED(style)) :
                         wxDialog(parent, ID_AUDIO, title, 
                         pos, size, wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODAL)
{
    wxBoxSizer        *dialogSizer =  new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer   *controlSizer = new wxFlexGridSizer(3,3,10,10);
    wxConfig          *config =       new wxConfig("iaxPhone");
    wxString          str;
    struct
    iaxc_audio_device *devices;
    int               nDevs;
    int               input, output, ring;
    int               i;
    long              caps;
    wxString          devname;

    config->SetPath("/");
    
    controlSizer->Add( 20,16);
    controlSizer->Add(280,16);
    controlSizer->Add(110,16);

    /* Input Device */
    controlSizer->Add(new wxStaticText(this, -1, "   Input Device:  "));
    controlSizer->Add(InDevice = new wxChoice(this, ID_INDEVICE,
                                     wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    /* Save */
    controlSizer->Add(Save = new wxButton(this, ID_SAVE, "Save"));

    /* Output Device */
    controlSizer->Add(new wxStaticText(this, -1, "   Output Device:  "));
    controlSizer->Add(OutDevice = new wxChoice(this, ID_OUTDEVICE,
                                      wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    /* OK */
    controlSizer->Add(new wxButton(this, wxID_OK, "Set"));

    /* Ring Device */
    controlSizer->Add(new wxStaticText(this, -1, "   Ring Device:  "));
    controlSizer->Add(RingDevice = new wxChoice(this, ID_RINGDEVICE,
                                       wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    /* CANCEL */
    controlSizer->Add(new wxButton(this, wxID_CANCEL, "Done"));

    /* Echo Cancel */
    controlSizer->Add( 20,16);
    controlSizer->Add(EchoCancel = new wxCheckBox(this, ID_ECHOCANCEL, 
                                       " Echo Cancel"));
    EchoCancel->SetValue(config->Read("Echo Cancel", 0l) != 0);

    iaxc_audio_devices_get(&devices, &nDevs, &input, &output, &ring);

    for(i=0; i<nDevs; i++) {
        caps =    devices->capabilities;
        devname = devices->name;

        if(caps & IAXC_AD_INPUT)
            InDevice->Append(devname);

        if(caps & IAXC_AD_OUTPUT)
            OutDevice->Append(devname);

        if(caps & IAXC_AD_RING)
            RingDevice->Append(devname);

        if(i == input)
            InDevice->SetStringSelection(devname);

        if(i == output)
            OutDevice->SetStringSelection(devname);

        if(i == ring)
            RingDevice->SetStringSelection(devname);

        devices++;
    }

    dialogSizer->Add(controlSizer);

    SetSizer(dialogSizer);
    SetAutoLayout(TRUE);
    Layout();
}

void AudioDialog::OnButton(wxCommandEvent &event)
{
    wxConfig *config = new wxConfig("iaxPhone");

    config->SetPath("/");

    switch(event.GetId())
    {
      case ID_SAVE:
           config->Write("Input Device",  InDevice->GetStringSelection());
           config->Write("Output Device", OutDevice->GetStringSelection());
           config->Write("Ring Device",   RingDevice->GetStringSelection());
           config->Write("Echo Cancel",   EchoCancel->GetValue());
           delete config;
           break;

      case wxID_OK:
           theFrame->SetAudioDeviceByName(InDevice->GetStringSelection(),
                                          OutDevice->GetStringSelection(),
                                          RingDevice->GetStringSelection());
           break;
    }
}

void IAXFrame::SetAudioDeviceByName(wxString inname, wxString outname, wxString ringname)
{
    struct iaxc_audio_device *devices;
    int                      nDevs;
    int                      i;
    int                      input  = 0;
    int                      output = 0;
    int                      ring   = 0;

    // Note that if we're called with an invalid devicename, the deviceID
    // stays 0, which equals default.

    iaxc_audio_devices_get(&devices, &nDevs, &input, &output, &ring);

    for(i=0; i<nDevs; i++) {
        if(devices->capabilities & IAXC_AD_INPUT) {
            if(inname.Cmp(devices->name) == 0)
                input = devices->devID;
        }

        if(devices->capabilities & IAXC_AD_OUTPUT) {
            if(outname.Cmp(devices->name) == 0)
                output = devices->devID;
        }

        if(devices->capabilities & IAXC_AD_RING) {
            if(ringname.Cmp(devices->name) == 0)
                ring = devices->devID;
        }
        devices++;
    }
    iaxc_audio_devices_set(input,output,ring);
}

BEGIN_EVENT_TABLE(AudioDialog, wxDialog)
    EVT_BUTTON(ID_SAVE, AudioDialog::OnButton)
    EVT_BUTTON(wxID_OK, AudioDialog::OnButton)
END_EVENT_TABLE()

ServerDialog::ServerDialog(wxWindow *parent, const wxString& title,
                           const wxPoint& pos, const wxSize& size, const long WXUNUSED(style)) :
                           wxDialog(parent, ID_SERVER, title, 
                           pos, size, wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODAL)
{
    wxBoxSizer      *dialogSizer  = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer *controlSizer = new wxFlexGridSizer(3,3,10,10);
    wxConfig        *config =       new wxConfig("iaxPhone");

    wxString        str;
    long            dummy;
    bool            bCont;
    
    controlSizer->Add( 20,16);
    controlSizer->Add(280,16);
    controlSizer->Add(110,16);

    /* Servers */
    controlSizer->Add(new wxStaticText(this, -1, "   Server:  "));
    controlSizer->Add(Server = new wxComboBox(this, ID_SERVER, "", 
                                   wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    config->SetPath("/Servers");
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
        Server->Append(str);
        bCont = config->GetNextGroup(str, dummy);
    }

    /* Add */
    controlSizer->Add(Add = new wxButton(this, ID_ADD, "Add"));

    /* AutoRegister */
    controlSizer->Add( 20,16);
    controlSizer->Add(AutoRegister = new wxCheckBox(this, ID_AUTOREGISTER, " Auto Register"));

    /* Remove */
    controlSizer->Add(Remove = new wxButton(this, ID_REMOVE, "Remove"));

    /* Host */
    controlSizer->Add(new wxStaticText(this, -1, "   Host:  "));
    controlSizer->Add(Host = new wxTextCtrl(this, ID_HOST, "", 
                                 wxDefaultPosition, wxDefaultSize),0,wxEXPAND);
    controlSizer->Add(110,1);

    /* Username */
    controlSizer->Add(new wxStaticText(this, -1, "   Username:  "));
    controlSizer->Add(Username = new wxTextCtrl(this, ID_USERNAME, "", 
                                     wxDefaultPosition, wxDefaultSize),0,wxEXPAND);
    /* Register */
    controlSizer->Add(new wxButton(this, wxID_OK, "Register"));

    /* Password */
    controlSizer->Add(new wxStaticText(this, -1, "   Password:  "));
    controlSizer->Add(Password = new wxTextCtrl(this, ID_PASSWORD, "", 
                                     wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    /* Done */
    controlSizer->Add(new wxButton(this, wxID_CANCEL, "Done"));

    dialogSizer->Add(controlSizer);

    SetSizer(dialogSizer);
    SetAutoLayout(TRUE);
    Layout();
}

void ServerDialog::OnButton(wxCommandEvent &event)
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString KeyPath;
    wxString ServerVal;

    ServerVal = Server->GetValue();

    KeyPath = "/Servers/" + ServerVal;
    config->SetPath(KeyPath);

    switch(event.GetId())
    {
      case ID_ADD:
           config->Write("Auto Register", AutoRegister->GetValue());
           config->Write("Host",          Host->GetValue());
           config->Write("Username",      Username->GetValue());
           config->Write("Password",      Password->GetValue());

           if(Server->FindString(ServerVal) < 0) {
             Server->Append(ServerVal);
           }
           break;

      case ID_REMOVE:
           config->DeleteGroup(KeyPath);
           // Remove it from the combobox, as well
           Server->Delete(Server->FindString(ServerVal));
           Host->Clear();
           Username->Clear();
           Password->Clear();
           break;

      case wxID_OK:
           theFrame->RegisterByName(ServerVal);
           break;
    }
    // Write out changes
    delete config;
}

void ServerDialog::OnComboBox(wxCommandEvent &event)
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString val;
    wxString KeyPath;

    // Update the Host/Username/Password boxes
    KeyPath = "/Servers/" + Server->GetStringSelection();
    config->SetPath(KeyPath);

    Host->SetValue(config->Read("Host", ""));
    Username->SetValue(config->Read("Username", ""));
    Password->SetValue(config->Read("Password", ""));
    AutoRegister->SetValue(config->Read("Auto Register", 0l) != 0);
}

BEGIN_EVENT_TABLE(ServerDialog, wxDialog)
    EVT_COMBOBOX(ID_SERVER, ServerDialog::OnComboBox)
    EVT_BUTTON(ID_ADD,      ServerDialog::OnButton)
    EVT_BUTTON(ID_REMOVE,   ServerDialog::OnButton)
    EVT_BUTTON(wxID_OK,     ServerDialog::OnButton)
END_EVENT_TABLE()

DialDialog::DialDialog(wxWindow *parent, const wxString& title,
                       const wxPoint& pos, const wxSize& size, const long WXUNUSED(style)) :
                       wxDialog(parent, ID_DIAL, title,
                       pos, size, wxDEFAULT_DIALOG_STYLE|wxDIALOG_MODAL)
{

    wxBoxSizer      *dialogSizer  = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer *controlSizer = new wxFlexGridSizer(3,3,10,10);
    wxConfig        *config =       new wxConfig("iaxPhone");

    wxString        str;
    long            dummy;
    bool            bCont;

    controlSizer->Add( 20,16);
    controlSizer->Add(280,16);
    controlSizer->Add( 96,16);

    /* Speed Dial */
    controlSizer->Add(new wxStaticText(this, -1, "   Speed Dial:  "));
    controlSizer->Add(SpeedDial = new wxComboBox(this, ID_SPEEDDIAL, "", 
                                      wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    config->SetPath("/Speed Dials");
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
      SpeedDial->Append(str);
      bCont = config->GetNextGroup(str, dummy);
    }

    /* Add */
    controlSizer->Add(Save = new wxButton(this, ID_ADD, "Add"));

    /* Server */
    controlSizer->Add(new wxStaticText(this, -1, "   Server:  "));
    controlSizer->Add(Server = new wxComboBox(this, ID_SERVER, "", 
                                   wxDefaultPosition, wxDefaultSize),0,wxEXPAND);

    config->SetPath("/Servers");
    bCont = config->GetFirstGroup(str, dummy);
    while ( bCont ) {
      Server->Append(str);
      bCont = config->GetNextGroup(str, dummy);
    }

    /* Remove */
    controlSizer->Add(Remove = new wxButton(this, ID_REMOVE, "Remove"));

    /* Extention */
    controlSizer->Add(new wxStaticText(this, -1, "   Extension:  "));
    controlSizer->Add(Extension = new wxTextCtrl(this, ID_EXTENSION, "", 
                                      wxDefaultPosition, wxDefaultSize),0,wxEXPAND);
    /* Dial */
    controlSizer->Add(new wxButton(this, ID_DIAL, "Dial"));

    /* Done */
    controlSizer->Add( 20,16);
    controlSizer->Add(280,16);
    controlSizer->Add(new wxButton(this, wxID_OK, "Done"));

    dialogSizer->Add(controlSizer);

    SetSizer(dialogSizer);
    SetAutoLayout(TRUE);
    Layout();
}

void DialDialog::OnComboBox(wxCommandEvent &event)
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString val;
    wxString KeyPath;

    // Update the Server/Extension boxes
    KeyPath = "/Speed Dials/" + SpeedDial->GetStringSelection();
    config->SetPath(KeyPath);

    Server->SetValue(config->Read("Server", ""));
    Extension->SetValue(config->Read("Extension", ""));
}

void DialDialog::OnButton(wxCommandEvent &event)
{
    wxConfig *config = new wxConfig("iaxPhone");
    wxString KeyPath;
    wxString SpeedDialVal;

    SpeedDialVal = SpeedDial->GetValue();

    KeyPath = "/Speed Dials/" + SpeedDialVal;
    config->SetPath(KeyPath);

    switch(event.GetId())
    {
      case ID_ADD:
           config->Write("Server",    Server->GetValue());
           config->Write("Extension", Extension->GetValue());

           if(SpeedDial->FindString(SpeedDialVal) < 0) {
             SpeedDial->Append(SpeedDialVal);
           }
           break;

      case ID_REMOVE:
           config->DeleteGroup(KeyPath);
           // Remove it from the combobox, as well
           SpeedDial->Delete(SpeedDial->FindString(SpeedDialVal));
           Server->SetValue("");
           Extension->Clear();
           break;

      case ID_DIAL:
           theFrame->DialBySpeedDialName(SpeedDial->GetValue());
    }
    // Write out changes
    delete config;
}

BEGIN_EVENT_TABLE(DialDialog, wxDialog)
    EVT_COMBOBOX(ID_SPEEDDIAL, DialDialog::OnComboBox)
    EVT_BUTTON(ID_ADD,         DialDialog::OnButton)
    EVT_BUTTON(ID_REMOVE,      DialDialog::OnButton)
    EVT_BUTTON(ID_DIAL,        DialDialog::OnButton)
END_EVENT_TABLE()
