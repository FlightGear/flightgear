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
    // DTMF events
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

    ID_SPEEDDIAL,

    // IAXFrame Buttons
    ID_DIAL = 100,
    ID_HANG,

    // File Menu items
    ID_QUIT = 200,

    // Options Menu items
    ID_PTT = 300,
    ID_MUTE,
    ID_SILENCE,
    ID_REGISTER,
    ID_AUDIO,
    ID_SERVER,
    ID_SPEED,

    ID_CALLS = 400,
    ID_REG,
        
    IAXCLIENT_EVENT = 500,

    ID_MAX
};

static char *buttonlabels[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#" };

class IAXTimer : public wxTimer
{
    public:
            void IAXTimer::Notify(); 
};


class RegList : public wxListCtrl
{
    public:
            RegList(wxWindow *parent, int regcount);
            void  RegList::OnSelect(wxListEvent &evt);
            void  RegList::OnSize(wxSizeEvent& evt);
            void  RegList::AutoSize(); 
            int   regcount;

    protected:
            DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(RegList, wxListCtrl)
    EVT_LIST_ITEM_SELECTED(ID_REG, RegList::OnSelect)
    EVT_SIZE(RegList::OnSize)
END_EVENT_TABLE()

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

class AudioDialog : public wxDialog
{
public:
    AudioDialog(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size,
            const long style = wxDEFAULT_DIALOG_STYLE);

protected:
    void OnButton(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()

private:
    enum
    {
        ID_INDEVICE	= 201,
        ID_OUTDEVICE,
        ID_RINGDEVICE,
        ID_ECHOCANCEL,
        ID_SAVE
    };

    wxChoice		*InDevice;
    wxChoice		*OutDevice;
    wxChoice		*RingDevice;
    wxCheckBox		*EchoCancel;
    wxButton		*Save;
};

class ServerDialog : public wxDialog
{
public:
    ServerDialog(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size,
            const long style = wxDEFAULT_DIALOG_STYLE);

protected:
    void OnComboBox(wxCommandEvent &event);
    void OnButton(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()

private:
    enum
    {
        ID_SERVER		= 201,
        ID_HOST,
        ID_USERNAME,
        ID_PASSWORD,
        ID_AUTOREGISTER,
        ID_ADD,
        ID_REMOVE
    };

    wxComboBox		*Server;
    wxTextCtrl		*Host;
    wxTextCtrl		*Username;
    wxTextCtrl		*Password;
    wxCheckBox		*AutoRegister;
    wxButton		*Add;
    wxButton		*Remove;
};

class DialDialog : public wxDialog
{
public:
    DialDialog(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size,
            const long style = wxDEFAULT_DIALOG_STYLE);

protected:
    void OnComboBox(wxCommandEvent &event);
    void OnButton(wxCommandEvent &event);
    DECLARE_EVENT_TABLE()

private:
    enum
    {
        ID_SPEEDDIAL	= 201,
        ID_SERVER,
        ID_EXTENSION,
        ID_ADD,
        ID_REMOVE,
        ID_DIAL
    };

    wxComboBox		*SpeedDial;
    wxComboBox		*Server;
    wxTextCtrl		*Extension;
    wxButton		*Save;
    wxButton		*Remove;
};

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
            void IAXFrame::OnAudioDialog(wxCommandEvent &evt);
            void IAXFrame::OnServerDialog(wxCommandEvent &evt);
            void IAXFrame::OnDialDialog(wxCommandEvent &evt);

            // utility methods
            void IAXFrame::RegisterByName(wxString RegName);
            void IAXFrame::DialBySpeedDialName(wxString name);
            void IAXFrame::SetAudioDeviceByName(wxString in, wxString out, wxString ring);

            // Handlers for library-initiated events
            void IAXFrame::HandleEvent(wxCommandEvent &evt);
            int IAXFrame::HandleIAXEvent(iaxc_event *e);
            int IAXFrame::HandleStatusEvent(char *msg);
            int IAXFrame::HandleLevelEvent(float input, float output);

            bool IAXFrame::GetPTTState();
            void IAXFrame::CheckPTT();
            void IAXFrame::SetPTT(bool state);

            wxGauge      *input; 
            wxGauge      *output; 
            IAXTimer     *timer;
            wxComboBox   *iaxServ;
            wxComboBox   *iaxDest;
            wxComboBox   *SpeedDial;
            wxStaticText *muteState;

            RegList      *registrations;
            IAXCalls     *calls;

            bool pttMode;  // are we in PTT mode?
            bool pttState; // is the PTT button pressed?
            bool silenceMode;  // are we in silence suppression mode?


    protected:
            DECLARE_EVENT_TABLE()

          #ifdef __WXGTK__
            GdkWindow *keyStateWindow;
          #endif
};
