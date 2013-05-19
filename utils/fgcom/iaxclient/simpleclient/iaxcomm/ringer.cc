//----------------------------------------------------------------------------------------
// Name:        ringer.cc
// Purpose:     Ringer class
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2004
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// GCC implementation
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma implementation "ringer.h"
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

#include "ringer.h"

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

#include <math.h>
#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif
#ifdef __WXGTK__
  #include <sys/ioctl.h>
  #include <sys/fcntl.h>
  #include <linux/kd.h>

static void Beep(int freq, int dur)
{
   int fd;
   int arg;

   fd = open("/dev/tty0", O_RDONLY);
   arg = (dur<<16)+(1193180/freq);
   ioctl(fd,KDMKTONE,arg);
}
#endif

//----------------------------------------------------------------------------------------
// Public methods
//----------------------------------------------------------------------------------------

void Ringer::Init(int F1, int F2, int Dur, int Len, int Repeat)
{
    // clear tone structures. (otherwise we free un-allocated memory in LoadTone)
    memset(&tone, 0, sizeof(tone));

    tone.len  = Len;
    tone.data = (short *)calloc(tone.len , sizeof(short));

    for( int i=0;i < Dur; i++ )
    {
        tone.data[i] = (short)(0x7fff*0.4*sin((double)i*F1*M_PI/8000))
                     + (short)(0x7fff*0.4*sin((double)i*F2*M_PI/8000));
    }

    tone.repeat = Repeat;
}

void Ringer::LoadTone(wxString Filename, int Repeat)
{
    wxFFile     fTone;

    if(Filename.IsEmpty())
        return;

    fTone.Open(Filename, _T("r"));

    if(!fTone.IsOpened())
        return;

    // Free old tone, if there was one
    if(tone.data != NULL)
        free(tone.data);

    tone.len  = fTone.Length();
    tone.data = (short *)calloc(tone.len , sizeof(short));
    fTone.Read(&tone.data[0], tone.len);
    fTone.Close();

    tone.repeat = Repeat;
}

void Ringer::Start(int ringdev)
{
    iaxc_play_sound(&tone, ringdev);
}

void Ringer::Stop()
{
    iaxc_stop_sound(tone.id);
}

