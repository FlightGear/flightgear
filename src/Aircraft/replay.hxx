// replay.hxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started July 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _FG_REPLAY_HXX
#define _FG_REPLAY_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <mutex>
#include <simgear/compiler.h>

#include <simgear/math/sg_types.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/io/iostreams/gzcontainerfile.hxx>
#include <simgear/io/HTTPFileRequest.hxx>

#include <MultiPlayer/multiplaymgr.hxx>

#include <deque>
#include <vector>

class FGFlightRecorder;

struct FGReplayData {

    double sim_time;
    // Our aircraft state.
    std::vector<char>   raw_data;
    
    // Incoming multiplayer messages.
    std::vector<std::shared_ptr<std::vector<char>>> multiplayer_messages;
    std::vector<char>                               extra_properties;
    std::map<std::string, std::string>              replay_extra_property_changes;
    std::vector<std::string>                        replay_extra_property_removals;
    
    // Updates static statistics defined below. 
    void UpdateStats();

    // Resets out static property nodes; to be called by fgStartNewReset().
    static void resetStatisticsProperties();
    
    FGReplayData();
    ~FGReplayData();
    
    size_t  m_bytes_raw_data = 0;
    size_t  m_bytes_multiplayer_messages = 0;
    size_t  m_num_multiplayer_messages = 0;
    
    // Statistics about replay data, also properties /sim/replay/datastats_*.
    static size_t   s_num;
    static size_t   s_bytes_raw_data;
    static size_t   s_bytes_multiplayer_messages;
    static size_t   s_num_multiplayer_messages;
    static SGPropertyNode_ptr   s_prop_num;
    static SGPropertyNode_ptr   s_prop_bytes_raw_data;
    static SGPropertyNode_ptr   s_prop_bytes_multiplayer_messages;
    static SGPropertyNode_ptr   s_prop_num_multiplayer_messages;
};

typedef struct {
    double sim_time;
    std::string message;
    std::string speaker;
} FGReplayMessages;

enum FGTapeType
{
    FGTapeType_NORMAL,
    FGTapeType_CONTINUOUS,
    FGTapeType_RECOVERY,
};

struct FGFrameInfo;
    
typedef std::deque < FGReplayData *> replay_list_type;
typedef std::vector < FGReplayMessages > replay_messages_type;

/**
 * A recording/replay module for FlightGear flights
 *
 */

class FGReplay : public SGSubsystem, SGPropertyChangeListener
{
public:
    FGReplay ();
    virtual ~FGReplay();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "replay"; }

    bool start(bool NewTape=false);

    bool saveTape(const SGPropertyNode* ConfigData);
    bool loadTape(const SGPropertyNode* ConfigData);
    
    // If filerequest is set, the local file is a Continuous recording and
    // it might increase in size as downloading progresses, so we need to
    // incrementally index the file until the file request has finished the
    // download.
    //
    bool loadTape(
            const SGPath& Filename,
            bool Preview,
            SGPropertyNode& MetaMeta,
            simgear::HTTP::FileRequestRef filerequest=nullptr
            );
    
    // Attempts to load Continuous recording header properties into
    // <properties>. If in is null we use internal std::fstream, otherwise we
    // use *in.
    //
    // Returns 0 on success, +1 if we may succeed after further download, or -1
    // if recording is not a Continuous recording.
    //
    static int loadContinuousHeader(const std::string& path, std::istream* in, SGPropertyNode* properties);

    // Prepends /sim/replay/tape-directory and/or appends .fgtape etc.
    //
    static std::string  makeTapePath(const std::string& tape_name);
    
private:
    void clear();
    FGReplayData* record(double time);
    void interpolate(double time, const replay_list_type &list);
    void replay(
            double time,
            size_t offset,
            size_t offset_old,
            bool replay_signals,
            bool replay_multiplayer,
            bool replay_extra_properties,
            int* xpos=nullptr,
            int* ypos=nullptr,
            int* xsize=nullptr,
            int* ysize=nullptr
            );
    void replay(
            double time,
            FGReplayData* pCurrentFrame,
            FGReplayData* pOldFrame=nullptr,
            int* xpos=nullptr,
            int* ypos=nullptr,
            int* xsize=nullptr,
            int* ysize=nullptr
            );
    void guiMessage(const char* message);
    void loadMessages();
    void fillRecycler();

    bool replay( double time );
    void replayMessage( double time );

    double get_start_time();
    double get_end_time();

    bool listTapes(bool SameAircraftFilter, const SGPath& tapeDirectory);
    bool saveTape(const SGPath& Filename, SGPropertyNode_ptr MetaData);
    
    // Build up in-memory cache of simulator time to file offset, so we can
    // handle random access.
    //
    // We also cache any frames that modify extra-properties.
    //
    // Can be called multiple times, e.g. if recording is being downlaoded.
    //
    void indexContinuousRecording(const void* data, size_t numbytes);
    
    // Callback for use with simgear::HTTP::FileRequest::setCallback().
    //
    static void call_indexContinuousRecording(void* ref, const void* data, size_t numbytes);
    
    SGPropertyNode_ptr continuousWriteHeader(
            std::ofstream&      out,
            const SGPath&       path,
            FGTapeType          tape_type
            );
    bool continuousWriteFrame(FGReplayData* r, std::ostream& out, SGPropertyNode_ptr meta);

    double sim_time;
    double last_mt_time;
    double last_lt_time;
    double last_msg_time;
    replay_messages_type::iterator current_msg;
    int last_replay_state;
    bool was_finished_already;

    replay_list_type short_term;
    replay_list_type medium_term;
    replay_list_type long_term;
    replay_list_type recycler;
    replay_messages_type replay_messages;

    SGPropertyNode_ptr disable_replay;
    SGPropertyNode_ptr replay_master;
    SGPropertyNode_ptr replay_master_eof;
    SGPropertyNode_ptr replay_time;
    SGPropertyNode_ptr replay_time_str;
    SGPropertyNode_ptr replay_looped;
    SGPropertyNode_ptr replay_duration_act;
    SGPropertyNode_ptr speed_up;
    SGPropertyNode_ptr replay_multiplayer;
    SGPropertyNode_ptr recovery_period;
    SGPropertyNode_ptr replay_error;
    SGPropertyNode_ptr m_record_normal_begin;   // Time of first in-memory recorded frame.
    SGPropertyNode_ptr m_record_normal_end;
    
    SGPropertyNode_ptr  m_sim_startup_xpos;
    SGPropertyNode_ptr  m_sim_startup_ypos;
    SGPropertyNode_ptr  m_sim_startup_xsize;
    SGPropertyNode_ptr  m_sim_startup_ysize;
    
    double replay_time_prev;    // Used to detect jumps while replaying.

    double m_high_res_time;    // default: 60 secs of high res data
    double m_medium_res_time;  // default: 10 mins of 1 fps data
    double m_low_res_time;     // default: 1 hr of 10 spf data
    // short term sample rate is as every frame
    double m_medium_sample_rate; // medium term sample rate (sec)
    double m_long_sample_rate;   // long term sample rate (sec)

    FGFlightRecorder*   m_pRecorder;
    
    FGMultiplayMgr*     m_MultiplayMgr;
    
    void valueChanged(SGPropertyNode * node);
    
    // Things for replaying from uncompressed fgtape file.
    std::ifstream                   m_continuous_in;
    bool                            m_continuous_in_multiplayer;
    bool                            m_continuous_in_extra_properties;
    std::mutex                      m_continuous_in_time_to_frameinfo_lock;
    std::map<double, FGFrameInfo>   m_continuous_in_time_to_frameinfo;
    SGPropertyNode_ptr              m_continuous_in_config;
    double                          m_continuous_in_time_last;
    double                          m_continuous_in_frame_time_last;
    
    std::ifstream                   m_continuous_indexing_in;
    std::streampos                  m_continuous_indexing_pos;
    
    // Only used for gathering statistics that are then written into
    // properties.
    //
    int m_num_frames_extra_properties = 0;
    int m_num_frames_multiplayer = 0;

    // For writing uncompressed fgtape file.
    SGPropertyNode_ptr  m_continuous_out_config;
    std::ofstream       m_continuous_out;
    int                 m_continuous_out_compression;
    int                 m_continuous_in_compression;
    
    SGPropertyNode_ptr  m_simple_time_enabled;
};

#endif // _FG_REPLAY_HXX
