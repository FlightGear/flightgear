#include <stdio.h>
#include <process.h>
//#include "sox.h"


HWND m_hwndMainDlg;
UINT_PTR m_iTimerId;
HBRUSH hBr;
HMENU hMenu;
int iSelLine;
int iTotCalls;
int piLines[3];
char szRegUser[50];
char szRegPass[50];
char szRegServer[60];
BOOL bPTT;
BOOL bSilence;
HWND hStBgClr;
HWND hStTxtClr;
COLORREF clrBg;
COLORREF clrTxt;
COLORREF clrTmpBg;
COLORREF clrTmpTxt;


void SelectLine(int line);


int TMessageBox(char *szMsg,int btn=MB_OK);
BOOL OnInitDialog();
BOOL OnSysCommand(UINT uCmd);
INT_PTR DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnBnDial();
void OnBnHangup();
int status_callback(char *msg);
int levels_callback(float input, float output);
void SendDTMF(char num);
BOOL OnTimer(UINT nIDEvent);
VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
void OnRegister();
INT_PTR RegDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR PrefDialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int HandleStateEvent(struct iaxc_ev_call_state c);
void ShowSoundSettings();
void ShowPreferences();
void SaveAudioSettings(char *szPlayback,char *szRecord,char *szRing);
BOOL RestoreAudioSettings(char *szPlayback,char* szRecord,char *szRing);
void SavePreferences(int iPTT,int iSilence);
BOOL RestorePreferences(int *iPTT,int *iSilence);
void TogglePTT(int iPTT);
void SelectColor(COLORREF *clr);
