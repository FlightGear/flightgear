// replay.cxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started July 2003.
// Updated by Thorsten Brehm, September 2011 and November 2012.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdio>
#include <float.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/iostreams/gzcontainerfile.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/commands.hxx>

#include <Main/fg_props.hxx>
#include <MultiPlayer/mpmessages.hxx>

#include "replay.hxx"
#include "flightrecorder.hxx"

using std::deque;
using std::vector;
using simgear::gzContainerReader;
using simgear::gzContainerWriter;

#if 1
    #define MY_SG_DEBUG SG_DEBUG
#else
    #define MY_SG_DEBUG SG_ALERT
#endif

/** Magic string to verify valid FG flight recorder tapes. */
static const char* const FlightRecorderFileMagic = "FlightGear Flight Recorder Tape";

void FGReplayData::UpdateStats()
{
    size_t  bytes_raw_data_old = m_bytes_raw_data;
    size_t  bytes_multiplayer_messages_old = m_bytes_multiplayer_messages;
    size_t  num_multiplayer_messages_old = m_num_multiplayer_messages;

    m_bytes_raw_data = raw_data.size();
    m_bytes_multiplayer_messages = 0;
    for ( auto m: multiplayer_messages) {
        m_bytes_multiplayer_messages += m->size();
    }
    m_num_multiplayer_messages = multiplayer_messages.size();

    // Update global stats by adding how much we have changed since last time
    // UpdateStats() was called.
    //
    s_bytes_raw_data += m_bytes_raw_data - bytes_raw_data_old;
    s_bytes_multiplayer_messages += m_bytes_multiplayer_messages - bytes_multiplayer_messages_old;
    s_num_multiplayer_messages += m_num_multiplayer_messages - num_multiplayer_messages_old;

    if (!s_prop_num) s_prop_num = fgGetNode("/sim/replay/datastats_num", true);
    if (!s_prop_bytes_raw_data) s_prop_bytes_raw_data = fgGetNode("/sim/replay/datastats_bytes_raw_data", true);
    if (!s_prop_bytes_multiplayer_messages) s_prop_bytes_multiplayer_messages = fgGetNode("/sim/replay/datastats_bytes_multiplayer_messages", true);
    if (!s_prop_num_multiplayer_messages) s_prop_num_multiplayer_messages = fgGetNode("/sim/replay/datastats_num_multiplayer_messages", true);

    s_prop_num->setLongValue(s_num);
    s_prop_bytes_raw_data->setLongValue(s_bytes_raw_data);
    s_prop_bytes_multiplayer_messages->setLongValue(s_bytes_multiplayer_messages);
    s_prop_num_multiplayer_messages->setLongValue(s_num_multiplayer_messages);
}

FGReplayData::FGReplayData()
{
    s_num += 1;
}

FGReplayData::~FGReplayData()
{
    s_bytes_raw_data -= m_bytes_raw_data;
    s_bytes_multiplayer_messages -= m_bytes_multiplayer_messages;
    s_num_multiplayer_messages -= m_num_multiplayer_messages;
    s_num -= 1;
}

void FGReplayData::resetStatisticsProperties()
{
    s_prop_num.reset();
    s_prop_bytes_raw_data.reset();
    s_prop_bytes_multiplayer_messages.reset();
    s_prop_num_multiplayer_messages.reset();
}

size_t   FGReplayData::s_num = 0;
size_t   FGReplayData::s_bytes_raw_data = 0;
size_t   FGReplayData::s_bytes_multiplayer_messages = 0;
size_t   FGReplayData::s_num_multiplayer_messages = 0;

SGPropertyNode_ptr   FGReplayData::s_prop_num;
SGPropertyNode_ptr   FGReplayData::s_prop_bytes_raw_data;
SGPropertyNode_ptr   FGReplayData::s_prop_bytes_multiplayer_messages;
SGPropertyNode_ptr   FGReplayData::s_prop_num_multiplayer_messages;


namespace ReplayContainer
{
    enum Type
    {
        Invalid    = -1,
        Header     = 0, /**< Used for initial file header (fixed identification string). */
        MetaData   = 1, /**< XML data / properties with arbitrary data, such as description, aircraft type, ... */
        Properties = 2, /**< XML data describing the recorded flight recorder properties.
                             Format is identical to flight recorder XML configuration. Also contains some
                             extra data to verify flight recorder consistency. */
        RawData    = 3  /**< Actual binary data blobs (the recorder's tape).
                             One "RawData" blob is used for each resolution. */
    };
}

/**
 * Constructor
 */

FGReplay::FGReplay() :
    sim_time(0),
    last_mt_time(0.0),
    last_lt_time(0.0),
    last_msg_time(0),
    last_replay_state(0),
    replay_time_prev(-1.0),
    m_high_res_time(60.0),
    m_medium_res_time(600.0),
    m_low_res_time(3600.0),
    m_medium_sample_rate(0.5), // medium term sample rate (sec)
    m_long_sample_rate(5.0),   // long term sample rate (sec)
    m_pRecorder(new FGFlightRecorder("replay-config")),
    m_MultiplayMgr(globals->get_subsystem<FGMultiplayMgr>())
{
    SGPropertyNode* continuous = fgGetNode("/sim/replay/record-continuous", true);
    SGPropertyNode* fdm = fgGetNode("/sim/signals/fdm-initialized", true);
    continuous->addChangeListener(this, true /*initial*/);
    fdm->addChangeListener(this, true /*initial*/);
}


static int PropertiesWrite(SGPropertyNode* root, std::ostream& out)
{
    stringstream buffer;
    writeProperties(buffer, root, true /*write_all*/);
    size_t buffer_len = buffer.str().size() + 1;
    out.write(reinterpret_cast<char*>(&buffer_len), sizeof(buffer_len));
    out.write(buffer.str().c_str(), buffer_len);
    return 0;
}

static int PropertiesRead(std::istream& in, SGPropertyNode* node)
{
    size_t  buffer_len;
    in.read(reinterpret_cast<char*>(&buffer_len), sizeof(buffer_len));
    std::vector<char>   buffer( buffer_len);
    in.read(&buffer.front(), buffer.size());
    readProperties(&buffer.front(), buffer.size()-1, node);
    return 0;
}

/* Reads uncompressed vector<char> from file. */
static void VectorRead(std::istream& in, std::vector<char>& out)
{
    size_t length;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    out.resize(length);
    in.read(&out.front(), length);
}

static void popupTip(const char* message, int delay)
{
    SGPropertyNode_ptr args(new SGPropertyNode);
    args->setStringValue("label", message);
    args->setIntValue("delay", delay);
    globals->get_commands()->execute("show-message", args);
}

enum FGTapeType
{
    FGTapeType_NORMAL,
    FGTapeType_CONTINUOUS,
    FGTapeType_RECOVERY,
};

// Returns a path using different formats depending on <type>:
//
//  FGTapeType_NORMAL:      <tape-directory>/<aircraft-type>-<date>-<time>.fgtape
//  FGTapeType_CONTINUOUS:  <tape-directory>/<aircraft-type>-<date>-<time>-continuous.fgtape
//  FGTapeType_RECOVERY:    <tape-directory>/<aircraft-type>-recovery.fgtape
//
static SGPath makeSavePath(FGTapeType type)
{
    const char* tapeDirectory = fgGetString("/sim/replay/tape-directory", "");
    const char* aircraftType  = fgGetString("/sim/aircraft", "unknown");
    
    SGPath  path = SGPath(tapeDirectory);
    path.append(aircraftType);
    path.concat("-");
    if (type == FGTapeType_RECOVERY) {
        path.concat("recovery");
    }
    else {
        time_t calendar_time = time(NULL);
        struct tm *local_tm;
        local_tm = localtime( &calendar_time );
        char time_str[256];
        strftime( time_str, 256, "%Y%m%d-%H%M%S", local_tm);
        path.concat(time_str);
    }
    if (type == FGTapeType_CONTINUOUS) {
        path.concat("-continuous");
    }
    path.concat(".fgtape");
    return path;
}

void FGReplay::valueChanged(SGPropertyNode * node)
{
    bool    prop_continuous = fgGetBool("/sim/replay/record-continuous");
    bool    prop_fdm = fgGetBool("/sim/signals/fdm-initialized");
    
    bool continuous = prop_continuous && prop_fdm;
    if (continuous == ((m_continuous_out.is_open()) ? true : false)) {
        // No change.
        return;
    }
    
    if (m_continuous_out) {
        // Stop existing continuous recording.
        m_continuous_out.close();
        popupTip("Continuous record to file stopped", 5 /*delay*/);
    }
    
    if (continuous) {
        // Start continuous recording.
        SGPropertyNode_ptr  MetaData;
        SGPropertyNode_ptr  Config;
        SGPath              path = makeSavePath(FGTapeType_CONTINUOUS);
        bool ok = continuousWriteHeader(m_continuous_out, MetaData, Config, path);
        if (!ok) {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to start continuous recording");
            popupTip("Continuous record to file failed to start", 5 /*delay*/);
            return;
        }
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Starting continuous recording to " << path);
        popupTip("Continuous record to file started", 5 /*delay*/);
    }
}

/**
 * Destructor
 */

FGReplay::~FGReplay()
{
    if (m_continuous_out.is_open()) {
        m_continuous_out.close();
    }
    clear();

    delete m_pRecorder;
    m_pRecorder = NULL;
}

/**
 * Clear all internal buffers.
 */
void
FGReplay::clear()
{
    while ( !short_term.empty() )
    {
        delete short_term.front();
        short_term.pop_front();
    }
    while ( !medium_term.empty() )
    {
        delete medium_term.front();
        medium_term.pop_front();
    }
    while ( !long_term.empty() )
    {
        delete long_term.front();
        long_term.pop_front();
    }
    while ( !recycler.empty() )
    {
        delete recycler.front();
        recycler.pop_front();
    }

    // clear messages belonging to old replay session
    fgGetNode("/sim/replay/messages", 0, true)->removeChildren("msg");
}

/** 
 * Initialize the data structures
 */

void
FGReplay::init()
{
    disable_replay       = fgGetNode("/sim/replay/disable",      true);
    replay_master        = fgGetNode("/sim/replay/replay-state", true);
    replay_time          = fgGetNode("/sim/replay/time",         true);
    replay_time_str      = fgGetNode("/sim/replay/time-str",     true);
    replay_looped        = fgGetNode("/sim/replay/looped",       true);
    replay_duration_act  = fgGetNode("/sim/replay/duration-act", true);
    speed_up             = fgGetNode("/sim/speed-up",            true);
    replay_multiplayer   = fgGetNode("/sim/replay/multiplayer",  true);
    recovery_period      = fgGetNode("/sim/replay/recovery-period", true);

    // alias to keep backward compatibility
    fgGetNode("/sim/freeze/replay-state", true)->alias(replay_master);

    reinit();
}

/** 
 * Reset replay queues.
 */

void
FGReplay::reinit()
{
    sim_time = 0.0;
    last_mt_time = 0.0;
    last_lt_time = 0.0;
    last_msg_time = 0.0;

    // Flush queues
    clear();
    m_pRecorder->reinit();

    m_high_res_time   = fgGetDouble("/sim/replay/buffer/high-res-time",    60.0);
    m_medium_res_time = fgGetDouble("/sim/replay/buffer/medium-res-time", 600.0); // 10 mins
    m_low_res_time    = fgGetDouble("/sim/replay/buffer/low-res-time",   3600.0); // 1 h
    // short term sample rate is as every frame
    m_medium_sample_rate = fgGetDouble("/sim/replay/buffer/medium-res-sample-dt", 0.5); // medium term sample rate (sec)
    m_long_sample_rate   = fgGetDouble("/sim/replay/buffer/low-res-sample-dt",    5.0); // long term sample rate (sec)

    fillRecycler();
    loadMessages();

    replay_master->setIntValue(0);
    disable_replay->setBoolValue(0);
    replay_time->setDoubleValue(0);
    replay_time_str->setStringValue("");
}

/** 
 * Bind to the property tree
 */

void
FGReplay::bind()
{
}

/** 
 *  Unbind from the property tree
 */

void
FGReplay::unbind()
{
    // nothing to unbind
}

void
FGReplay::fillRecycler()
{
    // Create an estimated nr of required ReplayData objects
    // 120 is an estimated maximum frame rate.
    int estNrObjects = (int) ((m_high_res_time*120) + (m_medium_res_time*m_medium_sample_rate) +
                             (m_low_res_time*m_long_sample_rate));
    for (int i = 0; i < estNrObjects; i++)
    {
        FGReplayData* r = new FGReplayData;
        if (r)
            recycler.push_back(r);
        else
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Out of memory!");
        }
    }
}

static void
printTimeStr(char* pStrBuffer,double _Time, bool ShowDecimal=true)
{
    if (_Time<0)
        _Time = 0;
    unsigned int Time = _Time*10;
    unsigned int h = Time/36000;
    unsigned int m = (Time % 36000)/600;
    unsigned int s = (Time % 600)/10;
    unsigned int d = Time % 10;

    int len;
    if (h>0)
        len = sprintf(pStrBuffer,"%u:%02u:%02u",h,m,s);
    else
        len = sprintf(pStrBuffer,"%u:%02u",m,s);

    if (len < 0)
    {
        *pStrBuffer = 0;
        return;
    }

    if (ShowDecimal)
        sprintf(&pStrBuffer[len],".%u",d);
}

void
FGReplay::guiMessage(const char* message)
{
    fgSetString("/sim/messages/copilot", message);
}

void
FGReplay::loadMessages()
{
    // load messages
    replay_messages.clear();
    simgear::PropertyList msgs = fgGetNode("/sim/replay/messages", true)->getChildren("msg");

    for (simgear::PropertyList::iterator it = msgs.begin();it != msgs.end();++it)
    {
        const char* msgText = (*it)->getStringValue("text", "");
        const double msgTime = (*it)->getDoubleValue("time", -1.0);
        const char* msgSpeaker = (*it)->getStringValue("speaker", "pilot");
        if ((msgText[0] != 0)&&(msgTime >= 0))
        {
            FGReplayMessages data;
            data.sim_time = msgTime;
            data.message = msgText;
            data.speaker = msgSpeaker;
            replay_messages.push_back(data);
        }
    }
    current_msg = replay_messages.begin();
}

void
FGReplay::replayMessage(double time)
{
    if (time < last_msg_time)
    {
        current_msg = replay_messages.begin();
    }

    // check if messages have to be triggered
    while ((current_msg != replay_messages.end())&&
           (time >= current_msg->sim_time))
    {
        // don't trigger messages when too long ago (fast-forward/skipped replay)
        if (time - current_msg->sim_time < 3.0)
        {
            fgGetNode("/sim/messages", true)->getNode(current_msg->speaker, true)->setStringValue(current_msg->message);
        }
        ++current_msg;
    }
    last_msg_time = time;
}

/** Start replay session
 */
bool
FGReplay::start(bool NewTape)
{
    // freeze the fdm, resume from sim pause
    double StartTime = get_start_time();
    double EndTime = get_end_time();
    was_finished_already = false;
    fgSetDouble("/sim/replay/start-time", StartTime);
    fgSetDouble("/sim/replay/end-time",   EndTime);
    char StrBuffer[30];
    printTimeStr(StrBuffer,StartTime,false);
    fgSetString("/sim/replay/start-time-str", StrBuffer);
    printTimeStr(StrBuffer,EndTime,false);
    fgSetString("/sim/replay/end-time-str",   StrBuffer);

    unsigned long buffer_elements =  short_term.size()+medium_term.size()+long_term.size();
    fgSetDouble("/sim/replay/buffer-size-mbyte",
                buffer_elements*m_pRecorder->getRecordSize() / (1024*1024.0));
    if ((fgGetBool("/sim/freeze/master"))||
        (0 == replay_master->getIntValue()))
        guiMessage("Replay active. 'Esc' to stop.");
    fgSetBool  ("/sim/freeze/master",     0);
    fgSetBool  ("/sim/freeze/clock",      0);
    if (0 == replay_master->getIntValue())
    {
        replay_master->setIntValue(1);
        if (NewTape)
        {
            // start replay at initial time, when loading a new tape
            replay_time->setDoubleValue(StartTime);
        }
        else
        {
            // start replay at "loop interval" when starting instant replay
            replay_time->setDoubleValue(-1);
        }
        replay_time_str->setStringValue("");
    }
    loadMessages();

    m_MultiplayMgr->ClearMotion();

    return true;
}

static const char* MultiplayerMessageCallsign(const std::vector<char>& raw_message)
{
    const T_MsgHdr* header = reinterpret_cast<const T_MsgHdr*>(&raw_message.front());
    return header->Callsign;
}

static bool CallsignsEqual(const std::vector<char>& a_message, const std::vector<char>& b_message)
{
    const char* a_callsign = MultiplayerMessageCallsign(a_message);
    const char* b_callsign = MultiplayerMessageCallsign(b_message);
    return 0 == strncmp(a_callsign, b_callsign, MAX_CALLSIGN_LEN);
}

// Moves all multiplayer packets from first item in <list> to second item in
// <list>, but only for callsigns that are not already mentioned in the second
// item's list.
//
// To be called before first item is discarded, so that we don't end up with
// large gaps in multiplayer information when replaying - this can cause
// multiplayer aircraft to spuriously disappear and reappear.
//
static void MoveFrontMultiplayerPackets(replay_list_type& list)
{
    auto it = list.begin();
    FGReplayData* a = *it;
    ++it;
    if (it == list.end()) {
        return;
    }
    FGReplayData* b = *it;

    // Copy all multiplayer packets in <a> that are for multiplayer aircraft
    // that are not in <b>, into <b>'s multiplayer_messages.
    //
    for (auto a_message: a->multiplayer_messages) {
        bool found = false;
        for (auto b_message: b->multiplayer_messages) {
            if (CallsignsEqual(*a_message, *b_message)) {
                found = true;
                break;
            }
        }
        if (!found) {
            b->multiplayer_messages.push_back(a_message);
        }
    }
}

/** 
 *  Update the saved data
 */

/** Save raw replay data in a separate container */
static bool
saveRawReplayData(gzContainerWriter& output, const replay_list_type& ReplayData, size_t RecordSize, bool multiplayer)
{
    // get number of records in this stream
    size_t Count = ReplayData.size();

    // write container header for raw data
    if (!output.writeContainerHeader(ReplayContainer::RawData, Count * RecordSize))
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to save replay data. Cannot write data container. Disk full?");
        return false;
    }

    // write the raw data (all records in the given list)
    replay_list_type::const_iterator it = ReplayData.begin();
    size_t CheckCount = 0;
    while ((it != ReplayData.end())&&
           !output.fail())
    {
        const FGReplayData* pRecord = *it++;
        assert(RecordSize == pRecord->raw_data.size());
        output.write(reinterpret_cast<const char*>(&pRecord->sim_time), sizeof(pRecord->sim_time));
        output.write(&pRecord->raw_data.front(), pRecord->raw_data.size());

        if (multiplayer) {
            size_t  num_messages = pRecord->multiplayer_messages.size();
            output.write(reinterpret_cast<const char*>(&num_messages), sizeof(num_messages));
            for ( auto message: pRecord->multiplayer_messages) {
                size_t message_size = message->size();
                output.write(reinterpret_cast<const char*>(&message_size), sizeof(message_size));
                output.write(&message->front(), message_size);
            }
        }
        CheckCount++;
    }

    // Did we really write as much as we intended?
    if (CheckCount != Count)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to save replay data. Expected to write " << Count << " records, but wrote " << CheckCount);
        return false;
    }

    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Saved " << CheckCount << " records of size " << RecordSize);
    return !output.fail();
}


// Sets things up for writing to a normal or continuous fgtape file.
//
//  Extra:
//      NULL or extra information when we are called from fgdata gui, e.g. with
//      the flight description entered by the user in the save dialogue.
//  path:
//      Path of fgtape file. We return nullptr if this file already exists.
//  Duration:
//      Duration of recording. Zero if we are starting a continuous recording.
//
//  Returns:
//      A new SGPropertyNode containing meta child with information about the
//      aircraft etc plus Extra's user-data if specified.
//
static SGPropertyNode_ptr saveSetup(
        const SGPropertyNode*   Extra,
        const SGPath&           path,
        double                  Duration
        )
{
    SGPropertyNode_ptr  MetaData;
    if (path.exists())
    {
        // same timestamp!?
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Error, flight recorder tape file with same name already exists: " << path);
        return MetaData;
    }
    
    MetaData = new SGPropertyNode();
    SGPropertyNode* meta = MetaData->getNode("meta", 0, true);

    // add some data to the file - so we know for which aircraft/version it was recorded
    meta->setStringValue("aircraft-type",           fgGetString("/sim/aircraft", "unknown"));
    meta->setStringValue("aircraft-description",    fgGetString("/sim/description", ""));
    meta->setStringValue("aircraft-fdm",            fgGetString("/sim/flight-model", ""));
    meta->setStringValue("closest-airport-id",      fgGetString("/sim/airport/closest-airport-id", ""));
    meta->setStringValue("aircraft-version",        fgGetString("/sim/aircraft-version", "(undefined)"));

    // add information on the tape's recording duration
    meta->setDoubleValue("tape-duration", Duration);
    char StrBuffer[30];
    printTimeStr(StrBuffer, Duration, false);
    meta->setStringValue("tape-duration-str", StrBuffer);

    // add simulator version
    copyProperties(fgGetNode("/sim/version", 0, true), meta->getNode("version", 0, true));
    if (Extra && Extra->getNode("user-data"))
    {
        copyProperties(Extra->getNode("user-data"), meta->getNode("user-data", 0, true));
    }

    bool multiplayer = fgGetBool("/sim/replay/multiplayer", false);
    meta->setBoolValue("multiplayer", multiplayer);

    // store replay messages
    copyProperties(fgGetNode("/sim/replay/messages", 0, true), MetaData->getNode("messages", 0, true));

    return MetaData;
}

// Opens continuous recording file and writes header.
//
// If MetaData is unset, we initialise it by calling saveSetup(). Otherwise
// should be already set up.
//
// If Config is unset, we make it point to a new node populated by
// m_pRecorder->getConfig(). Otherwise it should be already set up to point to
// such information.
//
// If path_override is not "", we use it as the path (instead of the path
// determined by saveSetup().
//
bool
FGReplay::continuousWriteHeader(
        std::ofstream&      out,
        SGPropertyNode_ptr& MetaData,
        SGPropertyNode_ptr& Config,
        const SGPath&       path
        )
{
    if (!MetaData) {
        MetaData = saveSetup(NULL /*Extra*/, path, 0 /*Duration*/);
        if (!MetaData) {
            return false;
        }
    }
    if (!Config) {
        Config = new SGPropertyNode;
        m_pRecorder->getConfig(Config.get());
    }
    
    out.open(path.c_str(), std::ofstream::binary | std::ofstream::trunc);
    out.write(FlightRecorderFileMagic, strlen(FlightRecorderFileMagic)+1);
    PropertiesWrite(MetaData, out);
    PropertiesWrite(Config, out);
    if (!out) {
        out.close();
        return false;
    }
    return true;
}


// Writes one frame of continuous record information.
//
bool
FGReplay::continuousWriteFrame(FGReplayData* r, std::ostream& out)
{
    out.write(reinterpret_cast<char*>(&r->sim_time), sizeof(r->sim_time));

    size_t    aircraft_data_size = r->raw_data.size();
    out.write(reinterpret_cast<char*>(&aircraft_data_size), sizeof(aircraft_data_size));
    out.write(&r->raw_data.front(), r->raw_data.size());

    size_t    multiplayer_num = r->multiplayer_messages.size();
    out.write(reinterpret_cast<char*>(&multiplayer_num), sizeof(multiplayer_num));
    for ( size_t i=0; i<multiplayer_num; ++i) {
        size_t  length = r->multiplayer_messages[i]->size();
        out.write(reinterpret_cast<char*>(&length), sizeof(length));
        out.write(&r->multiplayer_messages[i]->front(), length);
    }
    
    bool ok = true;
    if (!out) ok = false;
    return ok;
}


void
FGReplay::update( double dt )
{
    int current_replay_state = last_replay_state;
    timingInfo.clear();
    stamp("begin");

    if ( disable_replay->getBoolValue() )
    {
        if (fgGetBool("/sim/freeze/master",false)||
            fgGetBool("/sim/freeze/clock",false))
        {
            // unpause - disable the replay system in next loop
            fgSetBool("/sim/freeze/master",false);
            fgSetBool("/sim/freeze/clock",false);
            last_replay_state = 1;
        }
        else
        if ((replay_master->getIntValue() != 3)||
            (last_replay_state == 3))
        {
            // disable the replay system
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "End replay");
            current_replay_state = replay_master->getIntValue();
            replay_master->setIntValue(0);
            replay_time->setDoubleValue(0);
            replay_time_str->setStringValue("");
            disable_replay->setBoolValue(0);
            speed_up->setDoubleValue(1.0);
            speed_up->setDoubleValue(1.0);
            if (fgGetBool("/sim/replay/mute",false))
            {
                fgSetBool("/sim/sound/enabled",true);
                fgSetBool("/sim/replay/mute",false);
            }

            // Close any continuous replay file that we have open.
            //
            // This allows the user to use the in-memory record/replay system,
            // instead of replay always showing the continuous recording.
            //
            if (m_continuous_in.is_open()) {
                SG_LOG(SG_SYSTEMS, SG_DEBUG, "Unloading continuous recording");
                m_continuous_in.close();
                m_continuous_time_to_offset.clear();
            }
            assert(m_continuous_time_to_offset.empty());

            guiMessage("Replay stopped. Your controls!");
        }
    }

    int replay_state = replay_master->getIntValue();
    if ((replay_state == 0)&&
        (last_replay_state > 0))
    {
        if (current_replay_state == 3)
        {
            // take control at current replay position ("My controls!").
            // May need to uncrash the aircraft here :)
            fgSetBool("/sim/crashed", false);
        }
        else
        {
            // normal replay exit, restore most recent frame
            replay(DBL_MAX);
        }

        // replay is finished
        last_replay_state = replay_state;
        return;
    }

    // remember recent state
    last_replay_state = replay_state;

    switch(replay_state)
    {
        case 0:
            // replay inactive, keep recording
            break;
        case 1: // normal replay
        case 3: // prepare to resume normal flight at current replay position 
        {
            // replay active
            double current_time = replay_time->getDoubleValue();
            bool ResetTime = (current_time<=0.0);
            if (ResetTime)
            {
                // initialize start time
                double startTime = get_start_time();
                double endTime = get_end_time();
                fgSetDouble( "/sim/replay/start-time", startTime );
                fgSetDouble( "/sim/replay/end-time", endTime );
                double duration = 0;
                if (replay_duration_act->getBoolValue())
                    duration = fgGetDouble("/sim/replay/duration");
                if( duration && (duration < (endTime - startTime)) ) {
                    current_time = endTime - duration;
                } else {
                    current_time = startTime;
                }
            }
            if (current_time != replay_time_prev) {
                // User has skipped backwards or forward. Reset list of multiplayer aircraft.
                //
                m_MultiplayMgr->ClearMotion();
            }

            bool IsFinished = replay( current_time );
            if (IsFinished)
            {
                if (!was_finished_already)
                {
                    guiMessage("End of tape. 'Esc' to return.");
                    was_finished_already = true;
                }
                current_time = (replay_looped->getBoolValue()) ? -1 : get_end_time()+0.01;
            }
            else
            {
                current_time += dt;
                was_finished_already = false;
            }
            replay_time->setDoubleValue(current_time);
            replay_time_prev = current_time;
            char StrBuffer[30];
            printTimeStr(StrBuffer,current_time);
            replay_time_str->setStringValue((const char*)StrBuffer);

            // when time skipped (looped replay), trigger listeners to reset views etc
            if (ResetTime)
                replay_master->setIntValue(replay_state);

            if (replay_multiplayer->getIntValue()) {
                // Carry on recording while replaying.
                break;
            }
            else {
                // Don't record while replaying.
                return;
            }
        }
        case 2: // normal replay operation
            if (replay_multiplayer->getIntValue()) {
                // Carry on recording while replaying.
                break;
            }
            else {
                // Don't record while replaying.
                return;
            }
        default:
            throw sg_range_exception("unknown FGReplay state");
    }

    // flight recording

    // sanity check, don't collect data if FDM data isn't good
    if ((!fgGetBool("/sim/fdm-initialized", false))||(dt==0.0))
        return;

    {
        double new_sim_time = sim_time + dt;
        // don't record multiple records with the same timestamp (or go backwards in time)
        if (new_sim_time <= sim_time)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Time warp detected!");
            return;
        }
        sim_time = new_sim_time;
    }

    // We still record even if we are replaying, if /sim/replay/multiplayer is
    // true.
    //
    FGReplayData* r = record(sim_time);
    if (!r)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Out of memory!");
        return;
    }

    // update the short term list
    assert(r->raw_data.size() != 0);
    short_term.push_back( r );
    FGReplayData *st_front = short_term.front();

    if (!st_front)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Inconsistent data!");
    }
    
    if (m_continuous_out.is_open()) {
        continuousWriteFrame(r, m_continuous_out);
    }
    
    if (replay_state == 0)
    {
        // Update recovery tape.
        //
        double  recovery_period_s = recovery_period->getDoubleValue();
        if (recovery_period_s > 0) {
        
            static time_t   s_last_recovery = 0;
            time_t t = time(NULL);
            if (s_last_recovery == 0) {
                /* Don't save immediately. */
                s_last_recovery = t;
            }
            
            if (t - s_last_recovery >= recovery_period_s) {
                s_last_recovery = t;

                // We use static variables here to avoid calculating the same
                // data each time we are called.
                //
                static SGPath               path = makeSavePath(FGTapeType_RECOVERY);
                static SGPath               path_temp = SGPath( path.str() + "-");
                static SGPropertyNode_ptr   MetaData;
                static SGPropertyNode_ptr   Config;

                SG_LOG(SG_SYSTEMS, SG_BULK, "Creating recovery file: " << path);
                // We write to <path_temp> then rename to <path>, which should
                // guarantee that there is always a valid recovery tape even if
                // flightgear crashes or is killed while we are writing.
                //
                path.create_dir();
                (void) remove(path_temp.c_str());
                std::ofstream   out;
                bool ok = true;
                if (ok) ok = continuousWriteHeader(out, MetaData, Config, path_temp);
                if (ok) ok = continuousWriteFrame(r, out);
                out.close();
                if (ok) {
                    rename(path_temp.c_str(), path.c_str());
                }
                else {
                    std::string message = "Failed to update recovery file: " + path.str();
                    popupTip(message.c_str(), 3 /*delay*/);
                }
            }
        }
    }

    if ( sim_time - st_front->sim_time > m_high_res_time )
    {
        while ( sim_time - st_front->sim_time > m_high_res_time )
        {
            st_front = short_term.front();
            MoveFrontMultiplayerPackets(short_term);
            recycler.push_back(st_front);
            short_term.pop_front();
        }

        // update the medium term list
        if ( sim_time - last_mt_time > m_medium_sample_rate )
        {
            last_mt_time = sim_time;
            st_front = short_term.front();
            medium_term.push_back( st_front );
            short_term.pop_front();

            FGReplayData *mt_front = medium_term.front();
            if ( sim_time - mt_front->sim_time > m_medium_res_time )
            {
                while ( sim_time - mt_front->sim_time > m_medium_res_time )
                {
                    mt_front = medium_term.front();
                    MoveFrontMultiplayerPackets(medium_term);
                    recycler.push_back(mt_front);
                    medium_term.pop_front();
                }
                // update the long term list
                if ( sim_time - last_lt_time > m_long_sample_rate )
                {
                    last_lt_time = sim_time;
                    mt_front = medium_term.front();
                    long_term.push_back( mt_front );
                    medium_term.pop_front();

                    FGReplayData *lt_front = long_term.front();
                    if ( sim_time - lt_front->sim_time > m_low_res_time )
                    {
                        while ( sim_time - lt_front->sim_time > m_low_res_time )
                        {
                            lt_front = long_term.front();
                            MoveFrontMultiplayerPackets(long_term);
                            recycler.push_back(lt_front);
                            long_term.pop_front();
                        }
                    }
                }
            }
        }
    }

#if 0
    cout << "short term size = " << short_term.size()
         << "  time = " << sim_time - short_term.front().sim_time
         << endl;
    cout << "medium term size = " << medium_term.size()
         << "  time = " << sim_time - medium_term.front().sim_time
         << endl;
    cout << "long term size = " << long_term.size()
         << "  time = " << sim_time - long_term.front().sim_time
         << endl;
#endif
   //stamp("point_finished");
}

FGReplayData*
FGReplay::record(double time)
{
    FGReplayData* r = NULL;

    if (! recycler.empty())
    {
        r = recycler.front();
        recycler.pop_front();
    }

    return m_pRecorder->capture(time, r);
}

/** 
 * interpolate a specific time from a specific list
 */
void
FGReplay::interpolate( double time, const replay_list_type &list)
{
    // sanity checking
    if ( list.empty() )
    {
        // handle empty list
        return;
    } else if ( list.size() == 1 )
    {
        // handle list size == 1
        replay(time, list[0]);
        return;
    }

    unsigned int last = list.size() - 1;
    unsigned int first = 0;
    unsigned int mid = ( last + first ) / 2;

    bool done = false;
    while ( !done )
    {
        // cout << "  " << first << " <=> " << last << endl;
        if ( last == first ) {
            done = true;
        } else if ( list[mid]->sim_time < time && list[mid+1]->sim_time < time ) {
            // too low
            first = mid;
            mid = ( last + first ) / 2;
        } else if ( list[mid]->sim_time > time && list[mid+1]->sim_time > time ) {
            // too high
            last = mid;
            mid = ( last + first ) / 2;
        } else {
            done = true;
        }
    }

    replay(time, list[mid+1], list[mid]);
}

/** 
 *  Replay a saved frame based on time, interpolate from the two
 *  nearest saved frames.
 *  Returns true when replay sequence has finished, false otherwise.
 */

bool
FGReplay::replay( double time ) {
    // cout << "replay: " << time << " ";
    // find the two frames to interpolate between
    double t1, t2;

    replayMessage(time);
    
    if (!m_continuous_time_to_offset.empty()) {
        // Replay from uncompressed recording file.
        //
        auto p = m_continuous_time_to_offset.lower_bound(time);
        
        if (p == m_continuous_time_to_offset.end()) {
            // end.
            --p;
            replay( time, p->second);
            return true;
        }
        
        else if (p->first > time) {
            // Look for preceding item.
            if (p == m_continuous_time_to_offset.begin()) {
                replay(time, p->second);
                return false;
            }
            auto prev = p;
            --prev;
            replay( time, p->second, prev->second);
            return false;
        }
        else {
            // Exact match.
            replay(time, p->second);
            return false;
        }
    }

    if ( ! short_term.empty() ) {
        t1 = short_term.back()->sim_time;
        t2 = short_term.front()->sim_time;
        if ( time > t1 ) {
            // replay the most recent frame
            replay( time, short_term.back() );
            // replay is finished now
            return true;
        } else if ( time <= t1 && time >= t2 ) {
            interpolate( time, short_term );
        } else if ( ! medium_term.empty() ) {
            t1 = short_term.front()->sim_time;
            t2 = medium_term.back()->sim_time;
            if ( time <= t1 && time >= t2 )
            {
                replay(time, medium_term.back(), short_term.front());
            } else {
                t1 = medium_term.back()->sim_time;
                t2 = medium_term.front()->sim_time;
                if ( time <= t1 && time >= t2 ) {
                    interpolate( time, medium_term );
                } else if ( ! long_term.empty() ) {
                    t1 = medium_term.front()->sim_time;
                    t2 = long_term.back()->sim_time;
                    if ( time <= t1 && time >= t2 )
                    {
                        replay(time, long_term.back(), medium_term.front());
                    } else {
                        t1 = long_term.back()->sim_time;
                        t2 = long_term.front()->sim_time;
                        if ( time <= t1 && time >= t2 ) {
                            interpolate( time, long_term );
                        } else {
                            // replay the oldest long term frame
                            replay(time, long_term.front());
                        }
                    }
                } else {
                    // replay the oldest medium term frame
                    replay(time, medium_term.front());
                }
            }
        } else {
            // replay the oldest short term frame
            replay(time, short_term.front());
        }
    } else {
        // nothing to replay
        return true;
    }
    return false;
}

/** 
 * given two FGReplayData elements and a time, interpolate between them
 */
void
FGReplay::replay(double time, FGReplayData* pCurrentFrame, FGReplayData* pOldFrame)
{
    m_pRecorder->replay(time,pCurrentFrame,pOldFrame);
}


/* Reads a FGReplayData from uncompressed file. */
static std::unique_ptr<FGReplayData> ReadFGReplayData(std::ifstream& in, size_t pos)
{
    /* Need to clear any eof bit, otherwise seekg() will not work (which is
    pretty unhelpful). E.g. see:
        https://stackoverflow.com/questions/16364301/whats-wrong-with-the-ifstream-seekg
    */
    in.clear();
    in.seekg(pos);
    
    std::unique_ptr<FGReplayData>   ret(new FGReplayData);

    in.read(reinterpret_cast<char*>(&ret->sim_time), sizeof(ret->sim_time));
    VectorRead(in, ret->raw_data);

    /* Multiplayer information is a vector of vectors. */
    size_t  n;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    ret->multiplayer_messages.resize(n);
    for (size_t i=0; i<n; ++i) {
        ret->multiplayer_messages[i].reset(new std::vector<char>);
        VectorRead(in, *ret->multiplayer_messages[i]);
    }
    return ret;
}

/* Replays one iteration from uncompressed file. */
void FGReplay::replay(double time, size_t offset, size_t offset_old)
{
    SG_LOG(SG_SYSTEMS, SG_BULK,
            "FGReplay::replay():"
            << " time=" << time
            << " offset=" << offset
            << " offset_old=" << offset_old
            );
    std::unique_ptr<FGReplayData> replay_data = ReadFGReplayData(m_continuous_in, offset);
    std::unique_ptr<FGReplayData> replay_data_old;
    if (offset_old) {
        replay_data_old = ReadFGReplayData(m_continuous_in, offset_old);
    }
    m_pRecorder->replay(time, replay_data.get(), replay_data_old.get());
}

double
FGReplay::get_start_time()
{
    if (!m_continuous_time_to_offset.empty()) {
        double ret = m_continuous_time_to_offset.begin()->first;
        return ret;
    }
    
    if ( ! long_term.empty() )
    {
        return long_term.front()->sim_time;
    } else if ( ! medium_term.empty() )
    {
        return medium_term.front()->sim_time;
    } else if ( ! short_term.empty() )
    {
        return short_term.front()->sim_time;
    } else
    {
        return 0.0;
    }
}

double
FGReplay::get_end_time()
{
    if (!m_continuous_time_to_offset.empty()) {
        double ret = m_continuous_time_to_offset.rbegin()->first;
        return ret;
    }
    
    if ( ! short_term.empty() )
    {
        return short_term.back()->sim_time;
    } else
    {
        return 0.0;
    } 
}

/** Load raw replay data from a separate container */
static bool
loadRawReplayData(gzContainerReader& input, FGFlightRecorder* pRecorder, replay_list_type& ReplayData, size_t RecordSize, bool multiplayer)
{
    size_t Size = 0;
    simgear::ContainerType Type = ReplayContainer::Invalid;

    // write container header for raw data
    if (!input.readContainerHeader(&Type, &Size))
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Missing data container.");
        return false;
    }
    else
    if (Type != ReplayContainer::RawData)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Expected data container, got " << Type);
        return false;
    }
    
    SG_LOG(SG_SYSTEMS, SG_DEBUG, "multiplayer=" << multiplayer << " RecordSize=" << RecordSize << " Type=" << Type << " Size=" << Size);

    // read the raw data
    size_t Count = Size / RecordSize;
    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loading replay data. Container size is " << Size << ", record size " << RecordSize <<
           ", expected record count " << Count << ".");

    size_t CheckCount = 0;
    for (CheckCount=0; (CheckCount<Count)&&(!input.eof()); ++CheckCount)
    {
        FGReplayData* pBuffer = new FGReplayData;
        input.read(reinterpret_cast<char*>(&pBuffer->sim_time), sizeof(pBuffer->sim_time));
        pBuffer->raw_data.resize(RecordSize);
        input.read(&pBuffer->raw_data.front(), RecordSize);
        ReplayData.push_back(pBuffer);

        if (multiplayer) {
            size_t  num_messages = 0;
            input.read(reinterpret_cast<char*>(&num_messages), sizeof(num_messages));
            for (size_t i=0; i<num_messages; ++i) {
                size_t  message_size;
                input.read(reinterpret_cast<char*>(&message_size), sizeof(message_size));
                std::shared_ptr<std::vector<char>>  message(new std::vector<char>(message_size));
                input.read(&message->front(), message_size);
                pBuffer->multiplayer_messages.push_back( message);
            }
        }
    }

    // did we get all we have hoped for?
    if (CheckCount != Count)
    {
        if (input.eof())
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Unexpected end of file.");
        }
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to load replay data. Expected " << Count << " records, but got " << CheckCount);
        return false;
    }

    SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loaded " << CheckCount << " records of size " << RecordSize);
    return true;
}

/** Write flight recorder tape with given filename and meta properties to disk */
bool
FGReplay::saveTape(const SGPath& Filename, SGPropertyNode_ptr MetaData)
{
    bool ok = true;

    /* open output stream *******************************************/
    gzContainerWriter output(Filename, FlightRecorderFileMagic);
    
    if (!output.good())
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Cannot open file" << Filename);
        return false;
    }

    SG_LOG(SG_SYSTEMS, SG_DEBUG, "writing MetaData:");
    /* write meta data **********************************************/
    ok &= output.writeContainer(ReplayContainer::MetaData, MetaData.get());

    /* write flight recorder configuration **************************/
    SGPropertyNode_ptr Config;
    if (ok)
    {
        Config = new SGPropertyNode();
        m_pRecorder->getConfig(Config.get());
        ok &= output.writeContainer(ReplayContainer::Properties, Config.get());
    }
    
    /* write raw data ***********************************************/
    if (Config)
    {
        size_t RecordSize = Config->getIntValue("recorder/record-size", 0);
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Config:recorder/signal-count=" <<  Config->getIntValue("recorder/signal-count", 0)
               << " RecordSize: " << RecordSize);
        bool multiplayer = MetaData->getBoolValue("meta/multiplayer", 0);

        if (ok)
            ok &= saveRawReplayData(output, short_term,  RecordSize, multiplayer);
        if (ok)
            ok &= saveRawReplayData(output, medium_term, RecordSize, multiplayer);
        if (ok)
            ok &= saveRawReplayData(output, long_term,   RecordSize, multiplayer);
        Config = 0;
    }

    /* done *********************************************************/
    output.close();

    return ok;
}

/** Write flight recorder tape to disk. User/script command. */
bool
FGReplay::saveTape(const SGPropertyNode* Extra)
{
    SGPath path = makeSavePath(FGTapeType_NORMAL);
    SGPropertyNode_ptr MetaData = saveSetup(Extra, path, get_end_time()-get_start_time());
    bool ok = false;
    if (MetaData) {
        ok = saveTape(path, MetaData);
    }
    if (ok)
        guiMessage("Flight recorder tape saved successfully!");
    else
        guiMessage("Failed to save tape! See log output.");

    return ok;
}


/** Read a flight recorder tape with given filename from disk.
 * Copies MetaData's "meta" node into MetaMeta out-param.
 * Actual data and signal configuration is not read when in "Preview" mode.
 */
bool
FGReplay::loadTape(const SGPath& Filename, bool Preview, SGPropertyNode& MetaMeta)
{
    {
        /* Try to load as uncompressed first. */
        m_continuous_in.open( Filename.str());
        std::vector<char>   buffer(strlen( FlightRecorderFileMagic) + 1);
        m_continuous_in.read(&buffer.front(), buffer.size());
        if (strcmp(&buffer.front(), FlightRecorderFileMagic)) {
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "fgtape prefix doesn't match FlightRecorderFileMagic: " << Filename);
        }
        else {
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "fgtape is uncompressed: " << Filename);
            SGPropertyNode_ptr MetaDataProps = new SGPropertyNode();
            PropertiesRead(m_continuous_in, MetaDataProps.get());
            copyProperties(MetaDataProps->getNode("meta", 0, true), &MetaMeta);
            SGPropertyNode_ptr Config = new SGPropertyNode();
            PropertiesRead(m_continuous_in, Config.get());
            
            if (Preview) {
                m_continuous_in.close();
                return true;
            }
            
            m_pRecorder->reinit(Config);
            clear();
            fillRecycler();
            time_t t = time(NULL);
            size_t  pos = 0;
            for(;;)
            {
                pos = m_continuous_in.tellg();
                m_continuous_in.seekg(pos);
                double sim_time;
                m_continuous_in.read(reinterpret_cast<char*>(&sim_time), sizeof(sim_time));
                
                SG_LOG(SG_SYSTEMS, SG_BULK,
                        "pos=" << pos
                        << " m_continuous_in.tellg()=" << m_continuous_in.tellg()
                        << " sim_time=" << sim_time
                        );
                
                size_t  length_aircraft;
                m_continuous_in.read(reinterpret_cast<char*>(&length_aircraft), sizeof(length_aircraft));
                m_continuous_in.seekg(length_aircraft, std::ios_base::cur);
                
                size_t  num_multiplayer;
                m_continuous_in.read(reinterpret_cast<char*>(&num_multiplayer), sizeof(num_multiplayer));
                for ( size_t i=0; i<num_multiplayer; ++i) {
                    size_t length;
                    m_continuous_in.read(reinterpret_cast<char*>(&length), sizeof(length));
                    m_continuous_in.seekg(length, std::ios_base::cur);
                }
                
                SG_LOG(SG_SYSTEMS, SG_BULK, ""
                        << " pos=" << pos
                        << " sim_time=" << sim_time
                        << " length_aircraft=" << length_aircraft
                        << " num_multiplayer=" << num_multiplayer
                        );
                
                if (!m_continuous_in) {
                    break;
                }
                
                m_continuous_time_to_offset[sim_time] = pos;
            }
            t = time(NULL) - t;
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "Indexed uncompressed recording"
                    << ". time taken: " << t << "s"
                    << ". recording size: " << pos
                    << ". numrecording items: " << m_continuous_time_to_offset.size()
                    );
            start(true /*NewTape*/);
            return true;
        }
    }

    SG_LOG(SG_SYSTEMS, SG_DEBUG, "Filename=" << Filename);
    bool ok = true;

    /* open input stream ********************************************/
    gzContainerReader input(Filename, FlightRecorderFileMagic);
    if (input.eof() || !input.good())
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Cannot open file " << Filename);
        ok = false;
    }

    SGPropertyNode_ptr MetaDataProps = new SGPropertyNode();

    /* read meta data ***********************************************/
    if (ok)
    {
        char* MetaData = NULL;
        size_t Size = 0;
        simgear::ContainerType Type = ReplayContainer::Invalid;
        if (!input.readContainer(&Type, &MetaData, &Size) || Size<1)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                    << ". Invalid meta data.");
            ok = false;
        }
        else
        if (Type != ReplayContainer::MetaData)
        {
            SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Invalid header. Container type " << Type);
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename);
            ok = false;
        }
        else
        {
            try
            {
                readProperties(MetaData, Size-1, MetaDataProps);
                copyProperties(MetaDataProps->getNode("meta", 0, true), &MetaMeta);
            } catch (const sg_exception &e)
            {
              SG_LOG(SG_SYSTEMS, SG_ALERT, "Error reading flight recorder tape: " << Filename
                     << ", XML parser message:" << e.getFormattedMessage());
              ok = false;
            }
        }

        if (MetaData)
        {
            //printf("%s\n", MetaData);
            free(MetaData);
            MetaData = NULL;
        }
    }

    /* read flight recorder configuration **************************/
    if ((ok)&&(!Preview))
    {
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Loading flight recorder data...");
        char* ConfigXML = NULL;
        size_t Size = 0;
        simgear::ContainerType Type = ReplayContainer::Invalid;
        if (!input.readContainer(&Type, &ConfigXML, &Size) || Size<1)
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                   << ". Invalid configuration container.");
            ok = false;
        }
        else
        if ((!ConfigXML)||(Type != ReplayContainer::Properties))
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "File not recognized. This is not a valid FlightGear flight recorder tape: " << Filename
                   << ". Unexpected container type, expected \"properties\".");
            ok = false;
        }

        SGPropertyNode_ptr Config = new SGPropertyNode();
        if (ok)
        {
            try
            {
                readProperties(ConfigXML, Size-1, Config);
            } catch (const sg_exception &e)
            {
              SG_LOG(SG_SYSTEMS, SG_ALERT, "Error reading flight recorder tape: " << Filename
                     << ", XML parser message:" << e.getFormattedMessage());
              ok = false;
            }
            if (ok)
            {
                // reconfigure the recorder - and wipe old data (no longer matches the current recorder)
                m_pRecorder->reinit(Config);
                clear();
                fillRecycler();
            }
        }

        if (ConfigXML)
        {
            free(ConfigXML);
            ConfigXML = NULL;
        }

        /* read raw data ***********************************************/
        if (ok)
        {
            size_t RecordSize = m_pRecorder->getRecordSize();
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "RecordSize=" << RecordSize);
            size_t OriginalSize = Config->getIntValue("recorder/record-size", 0);
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "OriginalSize=" << OriginalSize);
            
            // check consistency - ugly things happen when data vs signals mismatch
            if ((OriginalSize != RecordSize)&&
                (OriginalSize != 0))
            {
                ok = false;
                SG_LOG(SG_SYSTEMS, SG_ALERT, "Error: Data inconsistency. Flight recorder tape has record size " << RecordSize
                       << ", expected size was " << OriginalSize << ".");
            }

            bool multiplayer = MetaMeta.getBoolValue("multiplayer", 0);
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "multiplayer=" << multiplayer);

            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, short_term,  RecordSize, multiplayer);
            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, medium_term, RecordSize, multiplayer);
            if (ok)
                ok &= loadRawReplayData(input, m_pRecorder, long_term,   RecordSize, multiplayer);

            // restore replay messages
            if (ok)
            {
                copyProperties(MetaDataProps->getNode("messages", 0, true),
                               fgGetNode("/sim/replay/messages", 0, true));
            }
            sim_time = get_end_time();
            // TODO we could (re)store these too
            last_mt_time = last_lt_time = sim_time;
        }
        /* done *********************************************************/
    }

    input.close();

    if (!Preview)
    {
        if (ok)
        {
            guiMessage("Flight recorder tape loaded successfully!");
            start(true);
        }
        else
            guiMessage("Failed to load tape. See log output.");
    }

    return ok;
}

/** List available tapes in current directory.
 * Limits to tapes matching current aircraft when SameAircraftFilter is enabled.
 */
bool
FGReplay::listTapes(bool SameAircraftFilter, const SGPath& tapeDirectory)
{
    const std::string& aircraftType = simgear::strutils::uppercase(fgGetString("/sim/aircraft", "unknown"));

    // process directory listing of ".fgtape" files
    simgear::Dir dir(tapeDirectory);
    simgear::PathList list =  dir.children(simgear::Dir::TYPE_FILE, ".fgtape");

    SGPropertyNode* TapeList = fgGetNode("/sim/replay/tape-list", true);
    TapeList->removeChildren("tape");
    int Index = 0;
    size_t l = aircraftType.size();
    for (simgear::PathList::iterator it = list.begin(); it!=list.end(); ++it)
    {
        SGPath file(it->file());
        std::string name(file.base());
        if ((!SameAircraftFilter)||
            (0==simgear::strutils::uppercase(name).compare(0,l, aircraftType)))
        {
            TapeList->getNode("tape", Index++, true)->setStringValue(name);
        }
    }

    return true;
}

/** Load a flight recorder tape from disk. User/script command. */
bool
FGReplay::loadTape(const SGPropertyNode* ConfigData)
{
    SGPath tapeDirectory(fgGetString("/sim/replay/tape-directory", ""));

    // see if shall really load the file - or just obtain the meta data for preview
    bool Preview = ConfigData->getBoolValue("preview", 0);

    // file/tape to be loaded
    std::string tape = ConfigData->getStringValue("tape", "");

    if (tape.empty())
    {
        if (!Preview)
            return listTapes(ConfigData->getBoolValue("same-aircraft", 0), tapeDirectory);
        return true;
    }
    else
    {
        SGPropertyNode* MetaMeta = fgGetNode("/sim/gui/dialogs/flightrecorder/preview", true);
        SGPath tapePath;
        if (simgear::strutils::ends_with(tape, ".fgtape"))
        {
            tapePath = tape;
        }
        else
        {
            tapePath = tapeDirectory;
            tapePath.append(tape);
            tapePath.concat(".fgtape");
        }
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Checking flight recorder file " << tapePath << ", preview: " << Preview);
        return loadTape(tapePath, Preview, *MetaMeta);
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGReplay> registrantFGReplay;
