// WinIAX.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"
#include <mmsystem.h>
#include "winiax.h"
#include "resource.h"
#include "iaxclient.h"
#include <commctrl.h>
#include <commdlg.h>
//#include <stdarg.h>
//#include <varargs.h>

#define APP_NAME "WinIAX Client"
#define LEVEL_MAX -10
#define LEVEL_MIN -50
#define TIMER_PROCESS_CALLS 1001
#define CALLER_ID "Vocalis IAX Client"

int iaxc_callback(iaxc_event e);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	WNDCLASSEX wnd;
	wnd.cbSize=sizeof(WNDCLASSEX);
	wnd.lpfnWndProc=(WNDPROC)DialogProc;
	wnd.lpszMenuName=MAKEINTRESOURCE(IDR_MENU1);
	wnd.hInstance=hInstance;
	int isz=sizeof(char);
//	wnd.hIcon=LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
//	wnd.hIconSm=LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
//	wnd.hIconSm=(HICON)LoadImage(hInstance ,MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR); 
 
//	memset(piActiveLines,-1,sizeof(int)*3);
//	iTotCalls=0;
	RegisterClassEx(&wnd);
	INITCOMMONCONTROLSEX cmtcl;
	cmtcl.dwSize=sizeof (INITCOMMONCONTROLSEX);
	cmtcl.dwICC=ICC_WIN95_CLASSES;
	InitCommonControlsEx(&cmtcl); 
	hBr=CreateSolidBrush(RGB(40,100,150));
	bPTT=FALSE;
	
//	m_hwndMainDlg=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MAIN_DLG),NULL,(DLGPROC)DialogProc);
	m_hwndMainDlg=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_MAIN_DLG),NULL,(DLGPROC)DialogProc);
	if(!m_hwndMainDlg)
	{	
//		GetLastError();
		TMessageBox("Couldnt create main dialog box\nAborting...");
		return 0;
	}
	hMenu=LoadMenu(NULL,MAKEINTRESOURCE(IDR_MENU1));
	SetMenu(m_hwndMainDlg,hMenu);
	ShowWindow(m_hwndMainDlg,SW_SHOW);
	SendMessage(GetDlgItem(m_hwndMainDlg,IDC_RD_LINE_1),BM_SETCHECK,1,0);
	
//	PBM_SETRANGE(0,LEVEL_MAX-LEVEL_MIN);

	
	MSG msg;
	while(GetMessage(&msg,NULL,0,0))
	{
		if(bPTT) // if the user wants PTT option.
		{
			// we toggle the PTT like this
			if(msg.message == WM_KEYDOWN) // when ctrl key is pressed
			{
				if(msg.wParam==VK_CONTROL)
					TogglePTT(1);
			}
			if(msg.message==WM_KEYUP) // when ctrl key goesup
			{
				if(msg.wParam==VK_CONTROL)
					TogglePTT(0);
			}
		}
		// Normal message processing.
		// IsDialogMessage for Dialogs to give tab stuff.
		if(!IsDialogMessage(m_hwndMainDlg,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
//	KillTimer(NULL,m_iTimerId);
	DestroyMenu(hMenu);
	iaxc_dump_call();
	iaxc_process_calls();
	for(int i=0;i<10;i++) {
	  iaxc_millisleep(100);
	  iaxc_process_calls();
	}
	iaxc_stop_processing_thread();
	iaxc_shutdown();
	return 0;
}


int TMessageBox(char *szMsg,int btn)
{
	return MessageBox(m_hwndMainDlg,szMsg,APP_NAME,btn);
}


INT_PTR DialogProc( HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	BOOL bRet=FALSE;
	switch (uMsg)
	{
	case WM_CHAR:
//		if((char)wParam=='F')
			TMessageBox("F Pressed");
		break;
	case WM_INITDIALOG:
		bRet=OnInitDialog();
		break;
	case WM_SYSCOMMAND:
		bRet=OnSysCommand(wParam);
		break;
	case WM_DESTROY:
		EndDialog(m_hwndMainDlg,0);
		PostQuitMessage(0);
		bRet=FALSE;
		break;
	case WM_TIMER:
		bRet=OnTimer(wParam);
		break;
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBr;
		break;
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam,RGB(40,100,150));
		SetTextColor((HDC)wParam,RGB(255,255,255));
		return (INT_PTR)hBr;
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_BN_DIAL:
				OnBnDial();
//				TMessageBox("Dial Clicked");
				break;
			case IDC_BN_HANGUP:
				OnBnHangup();
//				TMessageBox("Hangup Clicked");
				break;
			case IDC_BN_1:
				SendDTMF('1');
				break;
			case IDC_BN_2:
				SendDTMF('2');
				break;
			case IDC_BN_3:
				SendDTMF('3');
				break;
			case IDC_BN_4:
				SendDTMF('4');
				break;
			case IDC_BN_5:
				SendDTMF('5');
				break;
			case IDC_BN_6:
				SendDTMF('6');
				break;
			case IDC_BN_7:
				SendDTMF('7');
				break;
			case IDC_BN_8:
				SendDTMF('8');
				break;
			case IDC_BN_9:
				SendDTMF('9');
				break;
			case IDC_BN_ASTERISK:
				SendDTMF('*');
				break;
			case IDC_BN_0:
				SendDTMF('0');
				break;
			case IDC_BN_HASH:
				SendDTMF('#');
				break;
			case ID_FILE_REGISTER:
				OnRegister();
				break;
			case ID_FILE_EXIT:
				DestroyWindow(m_hwndMainDlg);
				break;
			case ID_TOOLS_SOUNDSETTINGS:
				ShowSoundSettings();
				break;
			case ID_TOOLS_PREFERENCES:
				ShowPreferences();
			case IDC_RD_LINE_1:
				SelectLine(0);
				break;
			case IDC_RD_LINE_2:
				SelectLine(1);
				break;
			case IDC_RD_LINE_3:
				SelectLine(2);
				break;
				
//			case IDC_PROG_INPUT:
//				break;
//			case IDC_PROG_OUTPUT:
//				bRet=TRUE;
//				break;
//			case IDC_E_IAX_URI:
//				break;
				
			}
		}
//		bRet=FALSE;
		break;
	}
	return bRet;
}

INT_PTR PrefDialogProc( HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	BOOL bRet=FALSE;
	switch (uMsg)
	{
			
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBr;
		break;
	case WM_CTLCOLORSTATIC:
		{
			SetBkColor((HDC)wParam,RGB(40,100,150));
			SetTextColor((HDC)wParam,RGB(255,255,255));
			return (INT_PTR)hBr;
		}
		break;
	case WM_INITDIALOG:
		{
			int iPTT=0;
			int iSilence=-99;
			RestorePreferences(&iPTT,&iSilence);
			SendMessage(GetDlgItem(hwndDlg,IDC_CHK_PTT),BM_SETCHECK,iPTT,0);
			if(iSilence>0)
				SendMessage(GetDlgItem(hwndDlg,IDC_CHK_SILENCE),BM_SETCHECK,iSilence,0);
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					int iPTT=0;
					if(SendMessage(GetDlgItem(hwndDlg,IDC_CHK_PTT),BM_GETCHECK,0,0)==BST_CHECKED)
						iPTT=1;
					int iSilence=0;
					if(SendMessage(GetDlgItem(hwndDlg,IDC_CHK_SILENCE),BM_GETCHECK,0,0)==BST_CHECKED)
						iSilence=1;
					SavePreferences(iPTT,iSilence);
					EndDialog(hwndDlg,wParam);
					bRet=TRUE;
				}
				break;
			case IDCANCEL:
				EndDialog(hwndDlg,wParam);
				bRet=TRUE;
				break;
			case IDC_BN_TEXT:
				SelectColor(&clrTmpTxt);
				break;
			case IDC_BN_BG:
				SelectColor(&clrTmpBg);
				break;
			}
			
		}
	}
	return bRet;
}

INT_PTR SoundDialogProc( HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	BOOL bRet=FALSE;
	switch (uMsg)
	{
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBr;
		break;
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam,RGB(40,100,150));
		SetTextColor((HDC)wParam,RGB(255,255,255));
		return (INT_PTR)hBr;
		break;
	case WM_INITDIALOG:
		{
			char szOut[MAX_PATH];
			char szIn[MAX_PATH];
			char szRing[MAX_PATH];
			ZeroMemory(szOut,MAX_PATH);
			ZeroMemory(szIn,MAX_PATH);
			ZeroMemory(szRing,MAX_PATH);
			// Get settings saved in Registry
			BOOL bReg=RestoreAudioSettings(szOut,szIn,szRing);

			// our comboboxes
			HWND hOut=GetDlgItem(hwndDlg,IDC_CMB_PLAYBACK);
			HWND hIn=GetDlgItem(hwndDlg,IDC_CMB_RECORDING);
			HWND hRing=GetDlgItem(hwndDlg,IDC_CMB_RINGTONE);
			struct iaxc_audio_device *devices; // we wont have tht many devices
			int nDevs=0;
			int in=0,out=0,ring=0;
			iaxc_audio_devices_get((struct iaxc_audio_device**)&devices,&nDevs,&in,&out,&ring);
			// first lets go thru out devices.			
			for(int i=0;i<nDevs;i++)
			{				
				if(devices[i].capabilities & IAXC_AD_OUTPUT)
				{
					SendMessage(hOut,CB_ADDSTRING,0,(LPARAM)devices[i].name);
					if(!strcmp(szOut,devices[i].name))
					{
						SetWindowText(hOut,devices[i].name);
					}
					else if(devices[i].capabilities & IAXC_AD_OUTPUT_DEFAULT && !bReg)
						SetWindowText(hOut,devices[i].name);
					
				}
				if(devices[i].capabilities & IAXC_AD_INPUT)
				{				
					SendMessage(hIn,CB_ADDSTRING,0,(LPARAM)devices[i].name);
					if(!strcmp(szIn,devices[i].name))
					{
						SetWindowText(hIn,devices[i].name);
					}
					else if(devices[i].capabilities & IAXC_AD_INPUT_DEFAULT)
						SetWindowText(hIn,devices[i].name);
				}
				if(devices[i].capabilities & IAXC_AD_RING)
				{				
					SendMessage(hRing,CB_ADDSTRING,0,(LPARAM)devices[i].name);
					if(!strcmp(szRing,devices[i].name))
					{
						SetWindowText(hRing,devices[i].name);
					}
					else if(devices[i].capabilities & IAXC_AD_RING_DEFAULT)
						SetWindowText(hRing,devices[i].name);
				}
			}			
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
				
					char szOut[MAX_PATH];
					char szIn[MAX_PATH];
					char szRing[MAX_PATH];
					// Select devices user has selected.
					int iOut=SendMessage(GetDlgItem(hwndDlg,IDC_CMB_PLAYBACK),CB_GETCURSEL,0,0);
					int iIn=SendMessage(GetDlgItem(hwndDlg,IDC_CMB_RECORDING),CB_GETCURSEL,0,0);					
					int iRing=SendMessage(GetDlgItem(hwndDlg,IDC_CMB_RINGTONE),CB_GETCURSEL,0,0);

					GetWindowText(GetDlgItem(hwndDlg,IDC_CMB_PLAYBACK),szOut,MAX_PATH);
//					SendMessage(GetDlgItem(hwndDlg,IDC_CMB_PLAYBACK),CB_GETLBTEXT,iOut,(LPARAM)szOut);
//					SendMessage(GetDlgItem(hwndDlg,IDC_CMB_RECORDING),CB_GETLBTEXT,iOut,(LPARAM)szIn);
					GetWindowText(GetDlgItem(hwndDlg,IDC_CMB_RECORDING),szIn,MAX_PATH);
//					SendMessage(GetDlgItem(hwndDlg,IDC_CMB_RINGTONE),CB_GETLBTEXT,iOut,(LPARAM)szRing);
					GetWindowText(GetDlgItem(hwndDlg,IDC_CMB_RINGTONE),szRing,MAX_PATH);
					iaxc_audio_devices_set(iIn,iOut,iRing);
					SaveAudioSettings(szOut,szIn,szRing);
					EndDialog(hwndDlg,wParam);
					bRet=TRUE;
				}
				break;
			case IDCANCEL:
				EndDialog(hwndDlg,wParam);
				bRet=TRUE;
				break;
			}
		}
		break;
	}
	return bRet;
}

INT_PTR RegDialogProc( HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	BOOL bRet=FALSE;
	switch (uMsg)
	{
	case WM_CTLCOLORDLG:
		return (INT_PTR)hBr;
		break;
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wParam,RGB(40,100,150));
		SetTextColor((HDC)wParam,RGB(255,255,255));
		return (INT_PTR)hBr;
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				GetDlgItemText(hwndDlg,IDC_E_USER,szRegUser,50);
				GetDlgItemText(hwndDlg,IDC_E_PASS,szRegPass,50);
				GetDlgItemText(hwndDlg,IDC_E_GATEWAY,szRegServer,60);
				EndDialog(hwndDlg,wParam);
				bRet=TRUE;
				break;
			case IDCANCEL:
				EndDialog(hwndDlg,wParam);
				bRet=TRUE;
				break;
			}
		}
	}
	return bRet;
}

BOOL OnInitDialog()
{
//	TMessageBox("OnInitDialog");
	// menu stuff
	double silence_threshold = -99;
//	if(!PlaySound("c:\\faizan\\iaxclient\\ring.wav",NULL,SND_FILENAME|SND_ASYNC))
//			  TMessageBox("Ünable to play sound");
//		  Sleep(3000);
//		  PlaySound(NULL,NULL,NULL);
	iaxc_initialize(AUDIO_INTERNAL_PA,3);
	//	iaxc_set_encode_format(IAXC_FORMAT_GSM);
	iaxc_set_silence_threshold(silence_threshold);

	iaxc_set_event_callback(iaxc_callback);

	iaxc_start_processing_thread();

	char szOut[MAX_PATH],szIn[MAX_PATH],szRing[MAX_PATH];
	if(RestoreAudioSettings(szOut,szIn,szRing))
	{
		struct iaxc_audio_device *devices; // we wont have tht many devices
		int nDevs=0;
		int in=0,out=0,ring=0;
		iaxc_audio_devices_get((struct iaxc_audio_device**)&devices,&nDevs,&in,&out,&ring);
		in=0,out=0,ring=0;
		int ins=0,outs=0,rings=0;
		// first lets go thru out devices.			
		for(int i=0;i<nDevs;i++)
		{				
			if(devices[i].capabilities & IAXC_AD_OUTPUT)
			{
				if(!strcmp(szOut,devices[i].name))
				{
					outs=out;
				}
				out++;
			}
			if(devices[i].capabilities & IAXC_AD_INPUT)
			{				
				if(!strcmp(szIn,devices[i].name))
				{
					ins=in;
				}
				in++;
			}
			if(devices[i].capabilities & IAXC_AD_RING)
			{				
				if(!strcmp(szRing,devices[i].name))
				{
					rings=ring;
				}
				ring++;
			}
		}	
		iaxc_audio_devices_set(ins,outs,rings);
	}
	int iPTT=0,iSilence=0;
	if(RestorePreferences(&iPTT,&iSilence))
	{
		bPTT=iPTT;
		bSilence=iSilence;
		if(bSilence)
			iaxc_set_silence_threshold(1);// auto silence
		else
			iaxc_set_silence_threshold(-99);
	}
	PostMessage(GetDlgItem(m_hwndMainDlg,IDC_PROG_OUTPUT),PBM_SETRANGE,0,LEVEL_MIN-LEVEL_MAX);
	PostMessage(GetDlgItem(m_hwndMainDlg,IDC_PROG_INPUT),PBM_SETRANGE,0,LEVEL_MIN-LEVEL_MAX);
	  
//	m_iTimerId=SetTimer(NULL,0,500,(TIMERPROC)TimerProc);
	return TRUE;
}

BOOL OnSysCommand(UINT uCmd)
{
	BOOL bRet=FALSE;
	switch (uCmd)
	{
	case SC_CLOSE:
		DestroyWindow(m_hwndMainDlg);
		bRet=TRUE;
		break;
	case SC_MINIMIZE:
		ShowWindow(m_hwndMainDlg,SW_MINIMIZE);
		bRet=TRUE;
		break;
	}
	return bRet;
}

void OnBnDial()
{
	char szString[MAX_PATH];
	GetDlgItemText(m_hwndMainDlg,IDC_E_IAX_URI,szString,MAX_PATH);
//	if(iTotCalls>0 && iTotCalls<3) // We do have a call so make another i guess
	iaxc_set_callerid(CALLER_ID, 0);
	iaxc_call(szString);
//	else if(iTotCalls>3)
//		return;
//	else
//	{
//		sprintf(szString,"%s@%s",szString,szRegServer);
//		iaxc_call(szString,CALLER_ID);
//	}
	iTotCalls++;
}

void OnBnHangup()
{
	if(iTotCalls<1)
		return;
	iaxc_dump_call();
	iTotCalls--;
}
int status_callback(char *msg)
{
	SetDlgItemText(m_hwndMainDlg,IDC_ST_STATUS,msg);
//	if(strstr(msg,"Hangup"
	return 1;
}

int levels_callback(float input, float output)
{
	int inputLevel,outputLevel;
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

	PostMessage(GetDlgItem(m_hwndMainDlg,IDC_PROG_OUTPUT),PBM_SETPOS,outputLevel,0);
	PostMessage(GetDlgItem(m_hwndMainDlg,IDC_PROG_INPUT),PBM_SETPOS,inputLevel,0);


	//char szStr[30];
	//sprintf(szStr,"Output %d,input %d",output,input);
	//SetDlgItemText(m_hwndMainDlg,IDC_ST_STATUS,szStr);
//	Set
//	theFrame->input->SetValue(inputLevel); 
//	theFrame->output->SetValue(outputLevel);
	return 1;
}

int iaxc_callback(iaxc_event e)
{
    switch(e.type) {
        case IAXC_EVENT_LEVELS:
            return levels_callback(e.ev.levels.input, e.ev.levels.output);
        case IAXC_EVENT_TEXT:
            return status_callback(e.ev.text.message);
      case IAXC_EVENT_STATE:
            return HandleStateEvent(e.ev.call);
//		case IAXC_EVENT_STATE:
//	    ret = status_callback(e->ev.call);
//	    break;

        default:
            return 0;  // not handled
    }
}


void SendDTMF(char num)
{
	if(iTotCalls>0) // We do have a call so just send DTMF there
		iaxc_send_dtmf(num);
	else
	{
		char szDialStr[100];
		GetDlgItemText(m_hwndMainDlg,IDC_E_IAX_URI,szDialStr,100);
		sprintf(szDialStr,"%s%c",szDialStr,num);
		SetDlgItemText(m_hwndMainDlg,IDC_E_IAX_URI,szDialStr);
	}
}

BOOL OnTimer(UINT nIDEvent)
{
	BOOL bRet=TRUE;
	if(nIDEvent==TIMER_PROCESS_CALLS)
	{
		iaxc_process_calls();
		bRet=FALSE;
	}
	return bRet;
}

VOID CALLBACK TimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{
	// Using thread instead of timer from now on.
	if(idEvent==m_iTimerId)
	{
		iaxc_process_calls();		
	}	
}

void OnRegister()
{
	if(DialogBox(NULL,MAKEINTRESOURCE(IDD_DLG_REGISTER),m_hwndMainDlg,(DLGPROC)RegDialogProc)==IDOK)
	{
//		TMessageBox("ÖK clicked");
		iaxc_register(szRegUser,szRegPass,szRegServer);
	}
}

int HandleStateEvent(struct iaxc_ev_call_state c)
{
//    wxListItem stateItem; // for all the state color

//    stateItem.m_itemId = c.callNo;

    // first, handle inactive calls
	char stStr[100];
    if(!(c.state & IAXC_CALL_STATE_ACTIVE)) {
	//fprintf(stderr, "state for item %d is free\n", c.callNo);
//	SetItem(c.callNo, 2, _T("No call") );
//	SetItem(c.callNo, 1, _T("") );
//	stateItem.SetTextColour(*wxLIGHT_GREY);
//	stateItem.SetBackgroundColour(*wxWHITE);
		sprintf(stStr,"Call No %d: Hungup by remote",c.callNo);
		SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,stStr);
    } 
	else
	{
		// set remote 
//	SetItem(c.callNo, 2, c.remote );
		
		BOOL outgoing = c.state & IAXC_CALL_STATE_OUTGOING;
		BOOL ringing = c.state & IAXC_CALL_STATE_RINGING;
		BOOL complete = c.state & IAXC_CALL_STATE_COMPLETE;
		BOOL rejected = c.state & 0; //IAXC_CALL_STATE_REJECTED;
		
		if( ringing && !outgoing ) {
//	    stateItem.SetTextColour(*wxBLACK);
//	    stateItem.SetBackgroundColour(*wxRED);
		}
		else
		{
//	    stateItem.SetTextColour(*wxBLUE);
//	    stateItem.SetBackgroundColour(*wxWHITE);
		}
		if(outgoing)
		{
// 			if((c.state & IAXC_CALL_STATE_AUTHREQ))
// 			{
// 				sprintf(stStr,"call no %d: Authentication required");
// 				TMessageBox(stStr);
// 			}
			if(ringing)
			{
				sprintf(stStr,"Ringing outbound call no %d",c.callNo);				
			}
//	    SetItem(c.callNo, 1, _T("r") );
			else if(complete)
				sprintf(stStr,"Call No %d: Answered",c.callNo);
//				SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,"Call answered");
			else if(rejected)
				sprintf(stStr,"Call No %d: Rejected",c.callNo);
//			else if((c.state & IAXC_CALL_STATE_DISCONNECTED))
//				sprintf(stStr,"Call No %d: Disconnected",c.callNo);
//				SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,"Call rejected");
//	    SetItem(c.callNo, 1, _T("o") );
			else // not accepted yet..
				sprintf(stStr,"Call no %d: Waiting for pickup");
			SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,stStr);
//				SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,"Waiting for pickup");
//	    SetItem(c.callNo, 1, _T("c") );
		}
		else	
		{
			if(ringing) 
			{
//		  if(!PlaySound("ring.wav",NULL,SND_FILENAME|SND_LOOP|SND_ASYNC|SND_NODEFAULT))
//			  TMessageBox("Ünable to play sound");
//		  Sleep(3000);
//		  PlaySound(NULL,NULL,NULL);
				SetDlgItemText(m_hwndMainDlg,IDC_ST_STATUS,"Incoming Call");

				char szMsg[100];
				sprintf(szMsg,"RING RING. Press OK to accept. CANCEL to reject.\nCallerid %s",c.remote);

				if(TMessageBox(szMsg,MB_OKCANCEL)==IDOK)
				{
					iaxc_answer_call(c.callNo);
					iaxc_select_call(c.callNo);
					SetDlgItemText(m_hwndMainDlg,IDC_ST_STATUS,"Call Answered");
					iTotCalls++;
				}
				else
				{
					iaxc_select_call(c.callNo);
					iaxc_reject_call();
				}
			}
//			else if((c.state & IAXC_CALL_STATE_DISCONNECTED))
//				sprintf(stStr,"Call No %d: Disconnected",c.callNo);
//	    SetItem(c.callNo, 1, _T("R") );
			else if(complete)
				SetDlgItemText(m_hwndMainDlg,IDC_ST_STATE,"Call answered");
//	    SetItem(c.callNo, 1, _T("I") );
			else // not accepted yet..  shouldn't happen!
				;
//	    SetItem(c.callNo, 1, _T("C") );
		} 
	// XXX do something more noticable if it's incoming, ringing!
    }
   
 //   SetItem( stateItem ); 
    
    // select if necessary
/*    if((c.state & IAXC_CALL_STATE_SELECTED) &&
	!(GetItemState(c.callNo,wxLIST_STATE_SELECTED|wxLIST_STATE_SELECTED))) 
    {
	//fprintf(stderr, "setting call %d to selected\n", c.callNo);
	SetItemState(c.callNo,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
    }
*/
    //AutoSize();
    //Refresh();

    return 0;
}
 
void SelectLine(int line)
{
	iaxc_select_call(line);
//		iaxc_select_call(line);
//		iSelLine=line;	
}

void ShowSoundSettings()
{
	DialogBox(NULL,MAKEINTRESOURCE(IDD_DLG_SOUND),m_hwndMainDlg,(DLGPROC)SoundDialogProc);
	
}

void ShowPreferences()
{
	DialogBox(NULL,MAKEINTRESOURCE(IDD_DLG_PREFERENCES),m_hwndMainDlg,(DLGPROC)PrefDialogProc);
}

void SaveAudioSettings(char *szPlayback,char *szRecord,char *szRing)
{
	HKEY hKey=NULL;
	DWORD dwDisp=0;
	if(RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\WinIAX",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)!=ERROR_SUCCESS)
	{
		TMessageBox("Couldn't save settings in Registry");
		return;
	}
	if(RegSetValueEx(hKey,"Playback Device",0,REG_SZ,(BYTE*)szPlayback,strlen(szPlayback))!=ERROR_SUCCESS)
	{
		TMessageBox("Couldn't save settings in Registry");
		goto CLEANUP;
	}
	if(RegSetValueEx(hKey,"Recording Device",0,REG_SZ,(BYTE*)szRecord,strlen(szRecord))!=ERROR_SUCCESS)
	{
		TMessageBox("Couldn't save settings in Registry");		
		goto CLEANUP;
	}
	if(RegSetValueEx(hKey,"Ringtone Device",0,REG_SZ,(BYTE*)szRing,strlen(szRing))!=ERROR_SUCCESS)
	{
		TMessageBox("Couldn't save settings in Registry");		
		goto CLEANUP;
	}
CLEANUP:
	RegCloseKey(hKey);	
}

BOOL RestoreAudioSettings(char *szPlayback,char* szRecord,char *szRing)
{
	HKEY hKey=NULL;
	DWORD dwType=0,dwLen=MAX_PATH;
	BOOL bRet=TRUE;
	if(RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\WinIAX",0,NULL,REG_OPTION_NON_VOLATILE,KEY_READ,NULL,&hKey,&dwType)!=ERROR_SUCCESS)
	{
//		TMessageBox("Couldn't save settings in Registry");
		return FALSE;
	}
	RegQueryValueEx(hKey,"Playback Device",0,&dwType,(BYTE*)szPlayback,&dwLen);
	dwLen=MAX_PATH;
	RegQueryValueEx(hKey,"Recording Device",0,&dwType,(BYTE*)szRecord,&dwLen);
	dwLen=MAX_PATH;
	RegQueryValueEx(hKey,"Ringtone Device",0,&dwType,(BYTE*)szRing,&dwLen);
//CLEANUP:
	RegCloseKey(hKey);	
	return bRet;
}

void SavePreferences(int iPTT,int iSilence)
{
	HKEY hKey=NULL;
	DWORD dwDisp=0;
	bPTT=iPTT;
	bSilence=iSilence;
	if(bSilence)
		iaxc_set_silence_threshold(1);
	else
		iaxc_set_silence_threshold(-99);
	if(RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\WinIAX",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)!=ERROR_SUCCESS)
	{
		return;
	}	
	RegSetValueEx(hKey,"PTT",0,REG_DWORD,(BYTE*)&iPTT,sizeof(int));
	RegSetValueEx(hKey,"Silence Filter",0,REG_DWORD,(BYTE*)&iSilence,sizeof(int));
	RegCloseKey(hKey);	
}

BOOL RestorePreferences(int *iPTT,int *iSilence)
{
	HKEY hKey=NULL;
	DWORD dwType;
	DWORD dwLen=0;
	BOOL bRet=TRUE;
	if(RegCreateKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\WinIAX",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwLen)!=ERROR_SUCCESS)
	{
		return FALSE;
	}
	dwLen=sizeof(DWORD);
	if(RegQueryValueEx(hKey,"PTT",0,&dwType,(BYTE*)iPTT,&dwLen)!=ERROR_SUCCESS)
	{
		bRet=FALSE;
		goto CLEANUP;
	}
	if(RegQueryValueEx(hKey,"Silence Filter",0,&dwType,(BYTE*)iSilence,&dwLen)!=ERROR_SUCCESS)
	{
		bRet=FALSE;
		goto CLEANUP;
	}
CLEANUP:
	RegCloseKey(hKey);
	return bRet;
}

void TogglePTT(int iPTT)
{
	if(iPTT)
	{
		iaxc_set_silence_threshold(-99);
		iaxc_set_audio_output(1);
	}
	else
	{
		iaxc_set_silence_threshold(0);  // mute input
		iaxc_set_audio_output(0);  // unmute output
	}
}

void SelectColor(COLORREF *clr)
{
	CHOOSECOLOR chlr;
	static COLORREF clrCust[16];
	ZeroMemory(&chlr,sizeof(CHOOSECOLOR));
	chlr.lStructSize=sizeof(CHOOSECOLOR);
	chlr.Flags=CC_FULLOPEN|CC_RGBINIT;
	chlr.lpCustColors=clrCust;
	
//	chlr.rgbResult=*clr;
	if(ChooseColor(&chlr))
	{
		*clr=chlr.rgbResult;
	}
}

