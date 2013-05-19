//----------------------------------------------------------------------------------------
// Name:        prefs,h
// Purpose:     Describes main dialog
// Author:      Michael Van Donselaar
// Modified by:
// Created:     2003
// Copyright:   (c) Michael Van Donselaar ( michael@vandonselaar.org )
// Licence:     GPL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Begin single inclusion of this .h file condition
//----------------------------------------------------------------------------------------

#ifndef _PREFS_H_
#define _PREFS_H_

//----------------------------------------------------------------------------------------
// GCC interface
//----------------------------------------------------------------------------------------

#if defined(__GNUG__) && ! defined(__APPLE__)
    #pragma interface "prefs.h"
#endif

//----------------------------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------------------------

#include "app.h"
#include "ringer.h"

void         SetCallerID(wxString name, wxString number);

class PrefsDialog : public wxDialog
{
public: 
    PrefsDialog( wxWindow* parent );
        
private:

    wxTextCtrl  *RingBack;
    wxTextCtrl  *RingTone;
    wxTextCtrl  *Intercom;
    wxButton    *SaveAudio;
    wxButton    *ApplyAudio;
    wxButton    *CancelAudio;

    wxTextCtrl  *Name;
    wxTextCtrl  *Number;
    wxButton    *SaveCallerID;
    wxButton    *ApplyCallerID;
    wxButton    *CancelCallerID;

    wxComboBox  *UseSkin;
    wxChoice    *DefaultAccount;
    wxTextCtrl  *IntercomPass;
    wxSpinCtrl  *nCalls;
    wxButton    *SaveMisc;
    wxButton    *ApplyMisc;
    wxButton    *CancelMisc;

    wxCheckBox  *AGC;
    wxCheckBox  *AAGC;
    wxCheckBox  *CN;
    wxCheckBox  *NoiseReduce;
    wxCheckBox  *EchoCancel;
    wxButton    *SaveFilters;
    wxButton    *ApplyFilters;
    wxButton    *CancelFilters;

    wxCheckBox     *AllowuLaw;
    wxCheckBox     *AllowaLaw;
    wxCheckBox     *AllowGSM;
    wxCheckBox     *AllowSpeex;
    wxCheckBox     *AllowiLBC;
    
    wxRadioButton  *PreferuLaw;
    wxRadioButton  *PreferaLaw;
    wxRadioButton  *PreferGSM;
    wxRadioButton  *PreferSpeex;
    wxRadioButton  *PreferiLBC;

    int             LocalPreferredBitmap;

    wxCheckBox     *SPXTune;
    wxCheckBox     *SPXEnhance;
    wxSpinCtrl     *SPXQuality;
    wxSpinCtrl     *SPXBitrate;
    wxSpinCtrl     *SPXABR;
    wxCheckBox     *SPXVBR;
    wxSpinCtrl     *SPXComplexity;

    wxButton    *SaveCodecs;
    wxButton    *ApplyCodecs;
    wxButton    *CancelCodecs;

    void         OnPreviewIntercom(wxCommandEvent &event);
    void         OnPreviewRingTone(wxCommandEvent &event);
    void         OnPreviewRingBack(wxCommandEvent &event);

    void         OnBrowse(wxCommandEvent &event);
    void         OnSaveAudio(wxCommandEvent &event);
    void         OnSaveCallerID(wxCommandEvent &event);
    void         OnSaveMisc(wxCommandEvent &event);
    void         OnSaveFilters(wxCommandEvent &event);
    void         OnSaveCodecs(wxCommandEvent &event);
    void         OnApplyAudio(wxCommandEvent &event);
    void         OnApplyCallerID(wxCommandEvent &event);
    void         OnApplyMisc(wxCommandEvent &event);
    void         OnApplyFilters(wxCommandEvent &event);
    void         OnApplyCodecs(wxCommandEvent &event);

    void         OnCodecPrefer(wxCommandEvent &event);
    void         OnCodecAllow(wxCommandEvent &event);

    void         OnSpeexTune(wxCommandEvent &event);
    void         OnSpeexTuneSpinEvent(wxSpinEvent &event);

    void         OnAudioDirty(wxCommandEvent &event);
    void         OnCallerIDDirty(wxCommandEvent &event);
    void         OnMiscDirty(wxCommandEvent &event);
    void         OnMiscDirtySpinEvent(wxSpinEvent &event);
    void         OnFiltersDirty(wxCommandEvent &event);

    DECLARE_EVENT_TABLE()

};

#endif  //_PREFS_H_
