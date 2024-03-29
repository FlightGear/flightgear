// flightrecorder.hxx
//
// Written by Thorsten Brehm, started August 2011.
//
// Copyright (C) 2011 Thorsten Brehm - brehmt (at) gmail com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef FLIGHTRECORDER_HXX_
#define FLIGHTRECORDER_HXX_

#include <simgear/props/props.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include "replay.hxx"

namespace FlightRecorder
{

    typedef enum
    {
        discrete    = 0,   // no interpolation
        linear      = 1,  // linear interpolation
        angular_rad = 2,  // angular interpolation, value in radians
        angular_deg = 3   // angular interpolation, value in degrees
    } TInterpolation;

    typedef struct
    {
        SGPropertyNode_ptr  Signal;
        TInterpolation      Interpolation;
    } TCapture;

    typedef std::vector<TCapture> TSignalList;

}

class FGFlightRecorder
{
public:
    FGFlightRecorder(const char* pConfigName);
    virtual ~FGFlightRecorder();

    void            reinit              (void);
    void            reinit              (SGPropertyNode_ptr ConfigNode);
    FGReplayData*   capture             (double SimTime, FGReplayData* pRecycledBuffer);
    
    // Updates main_window_* out-params if we find window move/resize events
    // and replay of such events is enabled.
    void            replay              (double SimTime, const FGReplayData* pNextBuffer,
                                         const FGReplayData* pLastBuffer,
                                         int* main_window_xpos,
                                         int* main_window_ypos,
                                         int* main_window_xsize,
                                         int* main_window_ysize
                                         );
    int             getRecordSize       (void) { return m_TotalRecordSize;}
    void            getConfig           (SGPropertyNode* root);
    void            resetExtraProperties();

private:
    SGPropertyNode_ptr getDefault(void);
    void initSignalList(const char* pSignalType, FlightRecorder::TSignalList& SignalList,
                        SGPropertyNode_ptr BaseNode);
    void processSignalList(const char* pSignalType, FlightRecorder::TSignalList& SignalList,
                           SGPropertyNode_ptr SignalListNode,
                           std::string PropPrefix="", int Count = 1);
    bool haveProperty(FlightRecorder::TSignalList& Capture,SGPropertyNode* pProperty);
    bool haveProperty(SGPropertyNode* pProperty);

    int  getConfig(SGPropertyNode* root, const char* typeStr, const FlightRecorder::TSignalList& SignalList);

    SGPropertyNode_ptr m_RecorderNode;
    SGPropertyNode_ptr m_ConfigNode;
    
    SGPropertyNode_ptr m_ReplayMultiplayer;
    SGPropertyNode_ptr m_ReplayExtraProperties;
    SGPropertyNode_ptr m_ReplayMainView;
    SGPropertyNode_ptr m_ReplayMainWindowPosition;
    SGPropertyNode_ptr m_ReplayMainWindowSize;
    
    SGPropertyNode_ptr m_RecordContinuous;
    SGPropertyNode_ptr m_RecordExtraProperties;
    
    SGPropertyNode_ptr m_LogRawSpeed;
    
    // This contains copy of all properties that we are recording, so that we
    // can send only differences.
    //
    SGPropertyNode_ptr m_RecordExtraPropertiesReference;
    
    FlightRecorder::TSignalList m_CaptureDouble;
    FlightRecorder::TSignalList m_CaptureFloat;
    FlightRecorder::TSignalList m_CaptureInteger;
    FlightRecorder::TSignalList m_CaptureInt16;
    FlightRecorder::TSignalList m_CaptureInt8;
    FlightRecorder::TSignalList m_CaptureBool;

    unsigned m_TotalRecordSize;
    std::string m_ConfigName;
    bool m_usingDefaultConfig;
    FGMultiplayMgr* m_MultiplayMgr;
};

#endif /* FLIGHTRECORDER_HXX_ */
