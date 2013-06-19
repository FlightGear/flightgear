//----------------------------------------------------------------------------------------
// Name:        frame.cc
// Purpose:     Main frame
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
    #pragma implementation "frame.h"
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

#include "frame.h"  

//----------------------------------------------------------------------------------------
// Remaining headers
// ---------------------------------------------------------------------------------------

#include "app.h"
#include "main.h"
#include "prefs.h"
#include "devices.h"
#include "directory.h"
#include "accounts.h"
#include "calls.h"
#include "dial.h"
#include "wx/image.h"
#include "wx/statusbr.h"
#include "wx/filesys.h"
#include "wx/fs_zip.h"
#include "wx/html/helpctrl.h"

static bool pttMode;      // are we in PTT mode?
static bool pttState;     // is the PTT button pressed?
static bool silenceMode;  // are we in silence suppression mode?

//----------------------------------------------------------------------------------------
// Event table: connect the events to the handler functions to process them
//----------------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU    (IAXCLIENT_EVENT,      MyFrame::HandleEvent)

    EVT_MENU    (XRCID("PTT"),         MyFrame::OnPTTChange)
    EVT_MENU    (XRCID("Silence"),     MyFrame::OnSilenceChange)

    EVT_MENU    (XRCID("Accounts"),    MyFrame::OnAccounts)
    EVT_MENU    (XRCID("Prefs"),       MyFrame::OnPrefs)
    EVT_MENU    (XRCID("Devices"),     MyFrame::OnDevices)
    EVT_MENU    (XRCID("Directory"),   MyFrame::OnDirectory)
    EVT_MENU    (XRCID("Exit"),        MyFrame::OnExit)
    EVT_MENU    (XRCID("About"),       MyFrame::OnAbout)
    EVT_MENU    (XRCID("Help"),        MyFrame::OnHelp)
    EVT_SIZE    (                      CallList::OnSize)  
#ifdef __WXMSW__
    EVT_ICONIZE (                      MyTaskBarIcon::OnIconize)
#endif
    EVT_BUTTON  (XRCID("0"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("1"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("2"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("3"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("4"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("5"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("6"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("7"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("8"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("9"),           MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("10"),          MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("11"),          MyFrame::OnOneTouch)
    EVT_BUTTON  (XRCID("KP0"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP1"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP2"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP3"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP4"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP5"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP6"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP7"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP8"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KP9"),         MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KPSTAR"),      MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("KPPOUND"),     MyFrame::OnKeyPad)
    EVT_BUTTON  (XRCID("Dial"),        MyFrame::OnDialDirect)
    EVT_BUTTON  (XRCID("Transfer"),    MyFrame::OnTransfer)
    EVT_BUTTON  (XRCID("Hold"),        MyFrame::OnHoldKey)
    EVT_BUTTON  (XRCID("Speaker"),     MyFrame::OnSpeakerKey)
    EVT_BUTTON  (XRCID("Hangup"),      MyFrame::OnHangup)

    EVT_CHOICE  (XRCID("Account"),     MyFrame::OnAccountChoice)

    EVT_COMMAND_SCROLL(XRCID("OutputSlider"), MyFrame::OnOutputSlider)
    EVT_COMMAND_SCROLL(XRCID("InputSlider"),  MyFrame::OnInputSlider)
    EVT_TEXT_ENTER    (XRCID("Extension"),    MyFrame::OnDialDirect)
END_EVENT_TABLE()

static void AddBook(wxHtmlHelpController *help, wxFileName filename);

//----------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------

MyFrame::MyFrame(wxWindow *parent)
{
    wxConfig   *config = theApp::getConfig();
    wxMenuBar  *aMenuBar;
    wxString    Name;
    MyTimer    *timer;

    MessageTicks = 0;

    // Load up this frame from XRC. [Note, instead of making a class's
    // constructor take a wxWindow* parent with a default value of NULL,
    // we could have just had designed MyFrame class with an empty
    // constructor and then written here:
    // wxXmlResource::Get()->LoadFrame(this, (wxWindow* )NULL, "MyFrame");
    // since this frame will always be the top window, and thus parentless.
    // However, the current approach has source code that can be recycled
    // in case code to moves to having an invisible frame as the top level window.

    wxXmlResource::Get()->LoadFrame(this, parent, _T("MyFrame"));

    //----Set the icon------------------------------------------------------------------
#ifdef __WXMSW__   
    SetIcon(wxICON(application));
#endif

    //----Add the menu------------------------------------------------------------------
    aMenuBar =  wxXmlResource::Get()->LoadMenuBar(_T("main_menubar"));
    SetMenuBar( aMenuBar);

    //----Add the statusbar-------------------------------------------------------------
    const int widths[] = {-1, 60};
    CreateStatusBar( 2 );
    SetStatusWidths(2, widths);

    //----Set some preferences ---------------------------------------------------------
    config->SetPath(_T("/Prefs"));

    RingOnSpeaker = config->Read(_T("RingOnSpeaker"), 0l);
    AGC           = config->Read(_T("AGC"), 0l);
    AAGC          = config->Read(_T("AAGC"), 1l);
    CN            = config->Read(_T("CN"), 1l);
    NoiseReduce   = config->Read(_T("NoiseReduce"), 1l);
    EchoCancel    = config->Read(_T("EchoCancel"), 0l);

    config->SetPath(_T("/Codecs"));

    AllowuLawVal    = config->Read(_T("AllowuLaw"),  0l);
    AllowaLawVal    = config->Read(_T("AllowaLaw"),  0l);
    AllowGSMVal     = config->Read(_T("AllowGSM"),   1l);
    AllowSpeexVal   = config->Read(_T("AllowSpeex"), 0l);
    AllowiLBCVal    = config->Read(_T("AllowiLBC"),  0l);
    PreferredBitmap = config->Read(_T("Preferred"),  IAXC_FORMAT_GSM);

    config->SetPath(_T("/Codecs/SpeexTune"));

    SPXTuneVal      = config->Read(_T("SPXTune"),        0l);
    SPXEnhanceVal   = config->Read(_T("SPXEnhance"),     1l);
    SPXQualityVal   = config->Read(_T("SPXQuality"),    -1l);
    SPXBitrateVal   = config->Read(_T("SPXBitrate"),     9l);
    SPXABRVal       = config->Read(_T("SPXABR"),         0l);
    SPXVBRVal       = config->Read(_T("SPXVBR"),         0l);
    SPXComplexityVal= config->Read(_T("SPXComplexity"),  3l);

    config->SetPath(_T("/Prefs"));

    //----Add the panel-----------------------------------------------------------------
    Name = config->Read(_T("UseSkin"), _T("default"));
    AddPanel(this, Name);

    pttMode = false;

    wxGetApp().InputDevice     = config->Read(_T("Input Device"), _T(""));
    wxGetApp().OutputDevice    = config->Read(_T("Output Device"), _T(""));
    wxGetApp().SpkInputDevice  = config->Read(_T("Speaker Input Device"),
                                              wxGetApp().InputDevice);
    wxGetApp().SpkOutputDevice = config->Read(_T("Speaker Output Device"), 
                                              wxGetApp().OutputDevice);
    wxGetApp().RingDevice      = config->Read(_T("Ring Device"), _T(""));

    ApplyFilters();
    ApplyCodecs();
    UsingSpeaker = false;

    if(OutputSlider != NULL)
        OutputSlider->SetValue(config->Read(_T("OutputLevel"), 70));

    if(InputSlider != NULL)
        InputSlider->SetValue(config->Read(_T("InputLevel"), 70));

#ifdef __WXGTK__
    // window used for getting keyboard state
    GdkWindowAttr attr;

    attr.window_type = GDK_WINDOW_CHILD;
    attr.x = 0;
    attr.y = 0;
    attr.width = 0;
    attr.height = 0;
    attr.wclass = GDK_INPUT_ONLY;
    attr.visual = NULL;
    attr.colormap = NULL;
    attr.event_mask = 0;
    keyStateWindow = gdk_window_new(NULL, &attr, 0);
#endif

    RePanel(Name);

    wxImage::AddHandler(new wxPNGHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);
    help = new wxHtmlHelpController;

    wxFileName filename = wxFileName(_T("iaxcomm.htb"));
    AddBook(help, filename);

    timer = new MyTimer();
    timer->Start(100);
}

void AddBook(wxHtmlHelpController *help, wxFileName filename)
{
#ifdef NOHELP
    if (filename.FileExists()) {
	help->AddBook(filename);
	return;
    }
#endif

#ifdef DATADIR
    filename = wxFileName(wxString(DATADIR, wxConvUTF8) + wxFILE_SEP_PATH +
			  _T("iaxcomm.htb"));

    if (filename.FileExists()) {
	help->AddBook(filename);
	return;
    }
#endif /* DATADIR */
}

void MyFrame::RePanel(wxString Name)
{
    aPanel->Destroy();
    AddPanel(this, Name);
    Layout();
}

void MyFrame::OnSize(wxSizeEvent &event)
{
    event.Skip();
}
    
void MyFrame::AddPanel(wxWindow *parent, wxString Name)
{
    wxBoxSizer *panelSizer;

    aPanel = new wxPanel(parent);
    aPanel = wxXmlResource::Get()->LoadPanel(parent, Name);
    if(aPanel == NULL)
        aPanel = wxXmlResource::Get()->LoadPanel(parent, wxT("default"));

    if(aPanel == NULL)
        wxLogError(_("Can't Load Panel in frame.cc"));

    //----Reach in for our controls-----------------------------------------------------
    Input        = XRCCTRL(*aPanel, "Input",        wxGauge);
    Output       = XRCCTRL(*aPanel, "Output",       wxGauge);
    OutputSlider = XRCCTRL(*aPanel, "OutputSlider", wxSlider);
    InputSlider  = XRCCTRL(*aPanel, "InputSlider",  wxSlider);
    Account      = XRCCTRL(*aPanel, "Account",      wxChoice);
    Extension    = XRCCTRL(*aPanel, "Extension",    wxComboBox);

    //----Insert the Calls listctrl into it's "unknown" placeholder---------------------
    Calls = new CallList(aPanel, wxGetApp().nCalls);

    if(Calls == NULL)
        wxLogError(_("Can't Load CallList in frame.cc"));

    wxXmlResource::Get()->AttachUnknownControl(_T("Calls"), Calls);

    ShowDirectoryControls();

    panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->Add(aPanel,1,wxEXPAND);

    SetSizer(panelSizer); 
    panelSizer->SetSizeHints(parent);
}

void MyFrame::ApplyFilters()
{
    // Clear these filters
    int flag = ~(IAXC_FILTER_AGC | IAXC_FILTER_AAGC | IAXC_FILTER_CN |
                 IAXC_FILTER_DENOISE | IAXC_FILTER_ECHO);

    flag = flag & iaxc_get_filters();

    if(AGC)
       flag |= IAXC_FILTER_AGC;

    if(AAGC)
       flag |= IAXC_FILTER_AAGC;

    if(CN)
       flag |= IAXC_FILTER_CN;

    if(NoiseReduce)
       flag |= IAXC_FILTER_DENOISE;

    if(EchoCancel)
       flag |= IAXC_FILTER_ECHO;

    iaxc_set_filters(flag);
}

void MyFrame::ApplyCodecs()
{
    int  Allowed = 0;

    if(AllowiLBCVal)
        Allowed |= IAXC_FORMAT_ILBC;
    
    if(AllowGSMVal)
        Allowed |= IAXC_FORMAT_GSM;
     
    if(AllowSpeexVal)   
        Allowed |= IAXC_FORMAT_SPEEX;
    
    if(AllowuLawVal)
        Allowed |= IAXC_FORMAT_ULAW;
    
    if(AllowaLawVal)
        Allowed |= IAXC_FORMAT_ALAW;

    iaxc_set_formats(PreferredBitmap, Allowed);

    if(SPXTuneVal) {
        iaxc_set_speex_settings(   (int)SPXEnhanceVal,
                                 (float)SPXQualityVal,
                                   (int)(SPXBitrateVal * 1000),
                                   (int)SPXVBRVal,
                                   (int)SPXABRVal,
                                   (int)SPXComplexityVal);

    }
}

MyFrame::~MyFrame()
{
#ifdef __WXMSW__
    delete wxGetApp().theTaskBarIcon;
#endif

#if 0
    // Getting rid of this block seems to be harmless, and also seems to get rid of the 
    // frequent hang when exiting on linux.
    iaxc_dump_all_calls();
    for(int i=0;i<10;i++) {
        iaxc_millisleep(100);
    }
    iaxc_stop_processing_thread();
#endif
    
    if(help != NULL)
        help->Quit();

    aPanel->Destroy();
}

void MyFrame::ShowDirectoryControls()
{
    wxConfig   *config = theApp::getConfig();
    wxButton   *ot;
    wxString    OTName;
    wxString    DialString;
    wxString    Name;
    wxString    Label;
    long        dummy;
    bool        bCont;

    //----Add Accounts-------------------------------------------------------------------
    if(Account != NULL) {
        Account->Clear();
        config->SetPath(_T("/Accounts"));
        bCont = config->GetFirstGroup(Name, dummy);
        while ( bCont ) {
            Account->Append(Name);
            bCont = config->GetNextGroup(Name, dummy);
        }

        Account->SetSelection(Account->FindString(wxGetApp().DefaultAccount));
    }

    //----Load up One Touch Keys--------------------------------------------------------
    config->SetPath(_T("/OT"));
    bCont = config->GetFirstGroup(OTName, dummy);
    while ( bCont ) {
#if defined(__UNICODE__)
        ot = ((wxButton *)((*aPanel).FindWindow(wxXmlResource::GetXRCID(OTName))));
#else
        ot = XRCCTRL(*aPanel, OTName, wxButton);
#endif
        if(ot != NULL) {
            Name = OTName + _T("/Name");
            Label = config->Read(Name, _T(""));
            if(!Label.IsEmpty()) {
                ot->SetLabel(Label);
            } else {
                ot->SetLabel(_T("OT") + OTName);
            }
            DialString = OTName + _T("/Extension");
            Label = config->Read(DialString, _T(""));
            if(!Label.IsEmpty()) {
                ot->SetToolTip(Label);
            }
        }
        bCont = config->GetNextGroup(OTName, dummy);
    }

    //----Load up Extension ComboBox----------------------------------------------------
    config->SetPath(_T("/PhoneBook"));
    bCont = config->GetFirstGroup(Name, dummy);
    while ( bCont ) {
        Extension->Append(Name);
        bCont = config->GetNextGroup(Name, dummy);
    }
}

void MyFrame::OnNotify()
{
    MessageTicks++;

    if(MessageTicks > 30) {
        MessageTicks = 0;
        wxGetApp().theFrame->SetStatusText(_T(""));
    }

    if(pttMode) CheckPTT(); 
}

void MyFrame::OnShow()
{
    Show(TRUE);
}

void MyFrame::OnSpeakerKey(wxCommandEvent &event)
{
    if(UsingSpeaker != true) {
        UsingSpeaker = true;
        SetAudioDevices(wxGetApp().SpkInputDevice,
                        wxGetApp().SpkOutputDevice,
                        wxGetApp().RingDevice);
    } else {
        UsingSpeaker = false;
        SetAudioDevices(wxGetApp().InputDevice,
                        wxGetApp().OutputDevice,
                        wxGetApp().RingDevice);
    }
}

void MyFrame::OnHoldKey(wxCommandEvent &event)
{
    int selected = iaxc_selected_call();

    if(selected < 0)
        return;

    iaxc_quelch(selected, 1);
    iaxc_select_call(-1);
}

void MyFrame::OnHangup(wxCommandEvent &event)
{
    iaxc_dump_call();
}

void MyFrame::OnQuit(wxEvent &event)
{
    Close(TRUE);
}

void MyFrame::OnOutputSlider(wxScrollEvent &event)
{
    int      pos = event.GetPosition();

    iaxc_output_level_set((double)pos/100.0);
}

void MyFrame::OnInputSlider(wxScrollEvent &event)
{
    int      pos = event.GetPosition();

    iaxc_input_level_set((double)pos/100.0);
}

void MyFrame::OnPTTChange(wxCommandEvent &event) {
    pttMode = event.IsChecked();
    
    if(pttMode) {
        SetPTT(GetPTTState()); 
    } else {
        SetPTT(true); 
        if(silenceMode) {
            iaxc_set_silence_threshold(DEFAULT_SILENCE_THRESHOLD);
            SetStatusText(_("VOX"),1);
        } else {
            iaxc_set_silence_threshold(-99);
            SetStatusText(_(""),1);
        }
        iaxc_set_audio_output(0);  // unmute output
    }
}

void MyFrame::OnSilenceChange(wxCommandEvent &event)
{
    // XXX get the actual state!
    silenceMode =  event.IsChecked();
    
    if(pttMode) return;

    if(silenceMode) {
        iaxc_set_silence_threshold(DEFAULT_SILENCE_THRESHOLD);
        SetStatusText(_("VOX"),1);
    } else {
        iaxc_set_silence_threshold(-99);
        SetStatusText(_(""),1);
    }
}

bool MyFrame::GetPTTState()
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

void MyFrame::CheckPTT()
{
    bool newState = GetPTTState();
    if(newState == pttState) return;
 
    SetPTT(newState);
}

void MyFrame::SetPTT(bool state)
{
    pttState = state;
    if(pttState) {
        iaxc_set_silence_threshold(-99); //unmute input
        iaxc_set_audio_output(1);  // unmute output
        SetStatusText(_("TALK"),1);
    } else {
        iaxc_set_silence_threshold(0);  // mute input
        iaxc_set_audio_output(0);  // mute output
        SetStatusText(_("MUTE"),1);
    }
}

void MyFrame::HandleEvent(wxCommandEvent &evt)
{
    iaxc_event *e = (iaxc_event *)evt.GetClientData();
    HandleIAXEvent(e);
    free (e);
}

int MyFrame::HandleIAXEvent(iaxc_event *e)
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
           ret = wxGetApp().theFrame->Calls->HandleStateEvent(e->ev.call);
           break;
      default:
           break;  // not handled
    }
     return ret;
}

int MyFrame::HandleStatusEvent(char *msg)
{
    MessageTicks = 0;
    wxGetApp().theFrame->SetStatusText(wxString(msg, *(wxGetApp().ConvIax)));
    return 1;
}

int MyFrame::HandleLevelEvent(float input, float output)
{
    static int lastInputLevel = 0;
    static int lastOutputLevel = 0;
    int inputLevel;
    int outputLevel;

    inputLevel = (int) input;
    if (inputLevel < LEVEL_MIN)
        inputLevel = LEVEL_MIN; 
    else if (inputLevel > LEVEL_MAX)
        inputLevel = LEVEL_MAX;
    inputLevel -= LEVEL_MIN;

    outputLevel = (int) output;
    if (outputLevel < LEVEL_MIN)
        outputLevel = LEVEL_MIN; 
    else if (outputLevel > LEVEL_MAX)
        outputLevel = LEVEL_MAX;
    outputLevel -= LEVEL_MIN;

    inputLevel *= 100;
    inputLevel /= (LEVEL_MAX - LEVEL_MIN);
    outputLevel *= 100;
    outputLevel /= (LEVEL_MAX - LEVEL_MIN);

    if (wxGetApp().theFrame->Input != NULL) {
      if (lastInputLevel != inputLevel) {
        wxGetApp().theFrame->Input->SetValue(inputLevel); 
        lastInputLevel = inputLevel;
      }
    }

    if (wxGetApp().theFrame->Output != NULL) {
      if (lastOutputLevel != outputLevel) {
        wxGetApp().theFrame->Output->SetValue(outputLevel); 
        lastOutputLevel = outputLevel;
      }
    }

    return 1;
}

//----------------------------------------------------------------------------------------
// Private methods
//----------------------------------------------------------------------------------------

void MyFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close(TRUE);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString  msg;
    char*     libver = (char *)malloc(256);
    
    libver = iaxc_version(libver);
    msg.Printf(_T("iaxComm version:\t%s\nlibiaxclient version:\t%s"), wxString(VERSION, wxConvUTF8).c_str(), wxString(libver, wxConvUTF8).c_str());
    wxMessageBox(msg, _("iaxComm"), wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnHelp(wxCommandEvent& WXUNUSED(event))
{
    help->DisplayContents();
}

void MyFrame::OnAccounts(wxCommandEvent& WXUNUSED(event))
{
    AccountsDialog dialog(this);
    dialog.ShowModal();
}

void MyFrame::OnPrefs(wxCommandEvent& WXUNUSED(event))
{
    PrefsDialog dialog(this);
    dialog.ShowModal();
}

void MyFrame::OnDevices(wxCommandEvent& WXUNUSED(event))
{
    DevicesDialog dialog(this);
    dialog.ShowModal();
}

void MyFrame::OnDirectory(wxCommandEvent& WXUNUSED(event))
{
    DirectoryDialog dialog(this);
    dialog.ShowModal();
}

void MyFrame::OnOneTouch(wxCommandEvent &event)
{
    wxConfig *config = theApp::getConfig();
    wxString  Message;
    int       OTNo;
    wxString  PathName;
    wxString  DialString;

    OTNo = event.GetId() - XRCID("0");

    PathName.Printf(_T("/OT/%d"), OTNo);
    config->SetPath(PathName);
    DialString = config->Read(_T("Extension"), _T(""));

    if(DialString.IsEmpty())
        return;

    // A DialString in quotes means look up name in phone book
    if(DialString.StartsWith(_T("\""))) {
        DialString = DialString.Mid(1, DialString.Len() -2);
        DialString = config->Read(_T("/PhoneBook/") + DialString + _T("/Extension"), _T(""));
    }
    Dial(DialString);
}

void MyFrame::OnKeyPad(wxCommandEvent &event)
{
    wxString  Message;
    wxChar    digit;
    int       OTNo;

    OTNo  = event.GetId() - XRCID("KP0");
    digit = wxT('0') + OTNo;

    if(OTNo == 10)
        digit = wxT('*');

    if(OTNo == 11)
        digit = wxT('#');

    iaxc_send_dtmf((char)digit);

    wxString SM;
    SM.Printf(_T("DTMF %c"), digit); 
    SetStatusText(SM); 

    Extension->SetValue(Extension->GetValue() + digit);
}

void MyFrame::OnDialDirect(wxCommandEvent &event)
{
    wxString  DialString;
    wxString  Value       = Extension->GetValue();
    wxConfig *config      = theApp::getConfig();

    DialString = config->Read(_T("/PhoneBook/") + Value + _T("/Extension"), _T(""));

    if(DialString.IsEmpty()) {
        Dial(Value);
    } else {
        Dial(DialString);
    }
    Extension->SetValue(_T(""));
}

void MyFrame::OnTransfer(wxCommandEvent &event)
{
    //Transfer
    int      selected = iaxc_selected_call();
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

void MyFrame::OnAccountChoice(wxCommandEvent &event)
{
    wxGetApp().DefaultAccount = Account->GetStringSelection();
}

void MyTimer::Notify()
{
    wxGetApp().theFrame->OnNotify();    
}

