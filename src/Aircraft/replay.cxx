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

#include <memory>
#include <iostream>
#include <streambuf>
#include <sstream>

#include <zlib.h>

#include <osgViewer/ViewerBase>

#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/iostreams/gzcontainerfile.hxx>
#include <simgear/io/iostreams/zlibstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/commands.hxx>

#include <Main/fg_props.hxx>
#include <MultiPlayer/mpmessages.hxx>
#include <Time/TimeManager.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/FGEventHandler.hxx>

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
    replay_error(fgGetNode("sim/replay/replay-error", true)),
    m_record_normal_begin(fgGetNode("sim/replay/record-normal-begin", true)),
    m_record_normal_end(fgGetNode("sim/replay/record-normal-end", true)),
    m_sim_startup_xpos(fgGetNode("sim/startup/xpos", true)),
    m_sim_startup_ypos(fgGetNode("sim/startup/ypos", true)),
    m_sim_startup_xsize(fgGetNode("sim/startup/xsize", true)),
    m_sim_startup_ysize(fgGetNode("sim/startup/ysize", true)),
    replay_time_prev(-1.0),
    m_high_res_time(60.0),
    m_medium_res_time(600.0),
    m_low_res_time(3600.0),
    m_medium_sample_rate(0.5), // medium term sample rate (sec)
    m_long_sample_rate(5.0),   // long term sample rate (sec)
    m_pRecorder(new FGFlightRecorder("replay-config")),
    m_MultiplayMgr(globals->get_subsystem<FGMultiplayMgr>()),
    m_simple_time_enabled(fgGetNode("/sim/time/simple-time/enabled", true))
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
    uint32_t    buffer_len = buffer.str().size() + 1;
    out.write(reinterpret_cast<char*>(&buffer_len), sizeof(buffer_len));
    out.write(buffer.str().c_str(), buffer_len);
    return 0;
}

static int PropertiesRead(std::istream& in, SGPropertyNode* node)
{
    uint32_t    buffer_len;
    in.read(reinterpret_cast<char*>(&buffer_len), sizeof(buffer_len));
    std::vector<char>   buffer( buffer_len);
    in.read(&buffer.front(), buffer.size());
    readProperties(&buffer.front(), buffer.size()-1, node);
    return 0;
}

/* Reads uncompressed vector<char> from file. Throws if length field is
longer than <max_length>. */
template<typename SizeType>
static SizeType VectorRead(std::istream& in, std::vector<char>& out, uint32_t max_length=(1<<31))
{
    SizeType    length;
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (sizeof(length) + length > max_length) {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "recording data vector too long."
                << " max_length=" << max_length
                << " sizeof(length)=" << sizeof(length)
                << " length=" << length
                );
        throw std::runtime_error("Failed to read vector in recording");
    }
    out.resize(length);
    in.read(&out.front(), length);
    return sizeof(length) + length;
}

static void popupTip(const char* message, int delay)
{
    SGPropertyNode_ptr args(new SGPropertyNode);
    args->setStringValue("label", message);
    args->setIntValue("delay", delay);
    globals->get_commands()->execute("show-message", args);
}


// Returns a path using different formats depending on <type>:
//
//  FGTapeType_NORMAL:      <tape-directory>/<aircraft-type>-<date>-<time>.fgtape
//  FGTapeType_CONTINUOUS:  <tape-directory>/<aircraft-type>-<date>-<time>-continuous.fgtape
//  FGTapeType_RECOVERY:    <tape-directory>/<aircraft-type>-recovery.fgtape
//
static SGPath makeSavePath(FGTapeType type, SGPath* path_timeless=nullptr)
{
    const char* tapeDirectory = fgGetString("/sim/replay/tape-directory", "");
    const char* aircraftType  = fgGetString("/sim/aircraft", "unknown");
    if (path_timeless) *path_timeless = "";
    SGPath  path = SGPath(tapeDirectory);
    path.append(aircraftType);
    if (type == FGTapeType_RECOVERY) {
        path.concat("-recovery");
    }
    else {
        if (path_timeless) *path_timeless = path;
        time_t calendar_time = time(NULL);
        struct tm *local_tm;
        local_tm = localtime( &calendar_time );
        char time_str[256];
        strftime( time_str, 256, "-%Y%m%d-%H%M%S", local_tm);
        path.concat(time_str);
    }
    if (type == FGTapeType_CONTINUOUS) {
        path.concat("-continuous");
        if (path_timeless) path_timeless->concat("-continuous");
    }
    path.concat(".fgtape");
    if (path_timeless) path_timeless->concat(".fgtape");
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
    
    if (m_continuous_out.is_open()) {
        // Stop existing continuous recording.
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Stopping continuous recording");
        m_continuous_out.close();
        popupTip("Continuous record to file stopped", 5 /*delay*/);
    }
    
    if (continuous) {
        // Start continuous recording.
        SGPath              path_timeless;
        SGPath              path = makeSavePath(FGTapeType_CONTINUOUS, &path_timeless);
        m_continuous_out_config = continuousWriteHeader(m_continuous_out, path, FGTapeType_CONTINUOUS);
        if (!m_continuous_out_config) {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to start continuous recording");
            popupTip("Continuous record to file failed to start", 5 /*delay*/);
            return;
        }
        
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Starting continuous recording");
        
        // Make a convenience link to the recording. E.g.
        // harrier-gr3-continuous.fgtape ->
        // harrier-gr3-20201224-005034-continuous.fgtape.
        //
        // Link destination is in same directory as link so we use leafname
        // path.file().
        //
        path_timeless.remove();
        bool ok = path_timeless.makeLink(path.file());
        if (!ok) {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to create link " << path_timeless.c_str() << " => " << path.file());
        }
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Starting continuous recording to " << path);
        if (m_continuous_out_compression) {
            popupTip("Continuous+compressed record to file started", 5 /*delay*/);
        }
        else {
            popupTip("Continuous record to file started", 5 /*delay*/);
        }
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
    replay_master_eof    = fgGetNode("/sim/replay/replay-state-eof", true);
    replay_time          = fgGetNode("/sim/replay/time",         true);
    replay_time_str      = fgGetNode("/sim/replay/time-str",     true);
    replay_looped        = fgGetNode("/sim/replay/looped",       true);
    replay_duration_act  = fgGetNode("/sim/replay/duration-act", true);
    speed_up             = fgGetNode("/sim/speed-up",            true);
    replay_multiplayer   = fgGetNode("/sim/replay/record-multiplayer",  true);
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

static void setTimeStr(const char* property_path, double t, bool show_decimal=false)
{
    char buffer[30];
    printTimeStr(buffer, t, show_decimal);
    fgSetString(property_path, buffer);
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
        replay_master_eof->setIntValue(0);
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
saveRawReplayData(gzContainerWriter& output, const replay_list_type& ReplayData, size_t RecordSize, SGPropertyNode* meta)
{
    // get number of records in this stream
    size_t Count = ReplayData.size();

    // write container header for raw data
    if (!output.writeContainerHeader(ReplayContainer::RawData, Count * RecordSize))
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to save replay data. Cannot write data container. Disk full?");
        return false;
    }

    // read the raw data (all records in the given list)
    replay_list_type::const_iterator it = ReplayData.begin();
    size_t CheckCount = 0;
    while ((it != ReplayData.end())&&
           !output.fail())
    {
        const FGReplayData* pRecord = *it++;
        assert(RecordSize == pRecord->raw_data.size());
        output.write(reinterpret_cast<const char*>(&pRecord->sim_time), sizeof(pRecord->sim_time));
        output.write(&pRecord->raw_data.front(), pRecord->raw_data.size());

        for (auto data: meta->getNode("meta")->getChildren("data")) {
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "data->getStringValue()=" << data->getStringValue());
            if (!strcmp(data->getStringValue(), "multiplayer")) {
                uint32_t length = 0;
                for (auto message: pRecord->multiplayer_messages){
                    length += sizeof(uint16_t) + message->size();
                }
                output.write(reinterpret_cast<const char*>(&length), sizeof(length));
                for (auto message: pRecord->multiplayer_messages) {
                    uint16_t message_size = message->size();
                    output.write(reinterpret_cast<const char*>(&message_size), sizeof(message_size));
                    output.write(&message->front(), message_size);
                }
                break;
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


/* streambuf that compresses using deflate(). */
struct compression_streambuf : std::streambuf
{
    compression_streambuf(
            std::ostream& out,
            size_t buffer_uncompressed_size,
            size_t buffer_compressed_size
            )
    :
    std::streambuf(),
    out(out),
    buffer_uncompressed(new char[buffer_uncompressed_size]),
    buffer_uncompressed_size(buffer_uncompressed_size),
    buffer_compressed(new char[buffer_compressed_size]),
    buffer_compressed_size(buffer_compressed_size)
    {
        zstream.zalloc = nullptr;
        zstream.zfree = nullptr;
        zstream.opaque = nullptr;
        
        zstream.next_in = nullptr;
        zstream.avail_in = 0;
        
        zstream.next_out = (unsigned char*) &buffer_compressed[0];
        zstream.avail_out = buffer_compressed_size;
        
        int e = deflateInit2(
                &zstream,
                Z_DEFAULT_COMPRESSION,
                Z_DEFLATED,
                -15 /*windowBits*/,
                8 /*memLevel*/,
                Z_DEFAULT_STRATEGY
                );
        if (e != Z_OK)
        {
            throw std::runtime_error("deflateInit2() failed");
        }
        // We leave space for one character to simplify overflow().
        setp(&buffer_uncompressed[0], &buffer_uncompressed[0] + buffer_uncompressed_size - 1);
    }
    
     // Flush compressed data to .out and reset zstream.next_out.
    void _flush()
    {
        // Send all data in .buffer_compressed to .out.
        size_t n = (char*) zstream.next_out - &buffer_compressed[0];
        out.write(&buffer_compressed[0], n);
        zstream.next_out = (unsigned char*) &buffer_compressed[0];
        zstream.avail_out = buffer_compressed_size;
    }
    
    /* Compresses specified bytes from buffer_uncompressed into
    buffer_compressed, flushing to .out as necessary. Returns true if we get
    EOF writing to .out. */
    bool _deflate(size_t n, bool flush)
    {
        assert(this->pbase() == &buffer_uncompressed[0]);
        zstream.next_in = (unsigned char*) &buffer_uncompressed[0];
        zstream.avail_in = n;
        for(;;)
        {
            if (!flush && !zstream.avail_in)   break;
            if (!zstream.avail_out) _flush();
            int e = deflate(&zstream, (!zstream.avail_in && flush) ? Z_FINISH : Z_NO_FLUSH);
            if (e != Z_OK && e != Z_STREAM_END)
            {
                throw std::runtime_error("zip_deflate() failed");
            }
            if (e == Z_STREAM_END)  break;
        }
        if (flush)  _flush();
        // We leave space for one character to simplify overflow().
        setp(&buffer_uncompressed[0], &buffer_uncompressed[0] + buffer_uncompressed_size - 1);
        if (!out) return true;  // EOF.
        return false;
    }
    
    int overflow(int c) override
    {
        // We've deliberately left space for one character, into which we write <c>.
        assert(this->pptr() == &buffer_uncompressed[0] + buffer_uncompressed_size - 1);
        *this->pptr() = (char) c;
        if (_deflate(buffer_uncompressed_size, false /*flush*/)) return EOF;
        return c;
    }
    
    int sync() override
    {
        _deflate(pptr() - &buffer_uncompressed[0], true /*flush*/);
        return 0;
    }
    
    ~compression_streambuf()
    {
        deflateEnd(&zstream);
    }
    
    std::ostream&           out;
    z_stream                zstream;
    std::unique_ptr<char[]> buffer_uncompressed;
    size_t                  buffer_uncompressed_size;
    std::unique_ptr<char[]> buffer_compressed;
    size_t                  buffer_compressed_size;
};


/* Accepts uncompressed data via .write(), operator<< etc, and writes
compressed data to the supplied std::ostream. */
struct compression_ostream : std::ostream
{
    compression_ostream(
            std::ostream& out,
            size_t buffer_uncompressed_size,
            size_t buffer_compressed_size
            )
    :
    std::ostream(&streambuf),
    streambuf(out, buffer_uncompressed_size, buffer_compressed_size)
    {
    }
    
    compression_streambuf   streambuf;
};



// Sets things up for writing to a normal or continuous fgtape file.
//
//  extra:
//      NULL or extra information when we are called from fgdata gui, e.g. with
//      the flight description entered by the user in the save dialogue.
//  path:
//      Path of fgtape file. We return nullptr if this file already exists.
//  duration:
//      Duration of recording. Zero if we are starting a continuous recording.
//  tape_type:
//      
//  Returns:
//      A new SGPropertyNode suitable as prefix of recording. If
//      extra:user-data exists, it will appear as meta/user-data.
//
static SGPropertyNode_ptr saveSetup(
        const SGPropertyNode*   extra,
        const SGPath&           path,
        double                  duration,
        FGTapeType              tape_type,
        int                     continuous_compression=0
        )
{
    SGPropertyNode_ptr  config;
    if (path.exists())
    {
        // same timestamp!?
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Error, flight recorder tape file with same name already exists: " << path);
        return config;
    }
    
    config = new SGPropertyNode;
    SGPropertyNode* meta = config->getNode("meta", 0 /*index*/, true /*create*/);

    // add some data to the file - so we know for which aircraft/version it was recorded
    meta->setStringValue("aircraft-type",           fgGetString("/sim/aircraft", "unknown"));
    meta->setStringValue("aircraft-description",    fgGetString("/sim/description", ""));
    meta->setStringValue("aircraft-fdm",            fgGetString("/sim/flight-model", ""));
    meta->setStringValue("closest-airport-id",      fgGetString("/sim/airport/closest-airport-id", ""));
    meta->setStringValue("aircraft-version",        fgGetString("/sim/aircraft-version", "(undefined)"));
    if (tape_type == FGTapeType_CONTINUOUS) {
        meta->setIntValue("continuous-compression", continuous_compression);
    }
    // add information on the tape's recording duration
    meta->setDoubleValue("tape-duration", duration);
    char StrBuffer[30];
    printTimeStr(StrBuffer, duration, false);
    meta->setStringValue("tape-duration-str", StrBuffer);

    // add simulator version
    copyProperties(fgGetNode("/sim/version", 0, true), meta->getNode("version", 0, true));
    if (extra && extra->getNode("user-data"))
    {
        copyProperties(extra->getNode("user-data"), meta->getNode("user-data", 0, true));
    }

    if (tape_type != FGTapeType_CONTINUOUS
            || fgGetBool("/sim/replay/record-signals", true)) {
        config->addChild("data")->setStringValue("signals");
    }
    
    if (tape_type == FGTapeType_CONTINUOUS) {
        if (fgGetBool("/sim/replay/record-multiplayer", false)) {
            config->addChild("data")->setStringValue("multiplayer");
        }
        if (0
                || fgGetBool("/sim/replay/record-extra-properties", false)
                || fgGetBool("/sim/replay/record-main-window", false)
                || fgGetBool("/sim/replay/record-main-view", false)
                ) {
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "Adding data[]=extra-properties."
                    << " record-extra-properties=" << fgGetBool("/sim/replay/record-extra-properties", false)
                    << " record-main-window=" << fgGetBool("/sim/replay/record-main-window", false)
                    << " record-main-view=" << fgGetBool("/sim/replay/record-main-view", false)
                    );
            config->addChild("data")->setStringValue("extra-properties");
        }
    }

    // store replay messages
    copyProperties(fgGetNode("/sim/replay/messages", 0, true), meta->getNode("messages", 0, true));

    return config;
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
SGPropertyNode_ptr FGReplay::continuousWriteHeader(
        std::ofstream&      out,
        const SGPath&       path,
        FGTapeType          tape_type
        )
{
    m_continuous_out_compression = fgGetInt("/sim/replay/record-continuous-compression");
    SGPropertyNode_ptr  config = saveSetup(NULL /*Extra*/, path, 0 /*Duration*/,
            tape_type, m_continuous_out_compression);
    SGPropertyNode* signals = config->getNode("signals", true /*create*/);
    m_pRecorder->getConfig(signals);
    
    out.open(path.c_str(), std::ofstream::binary | std::ofstream::trunc);
    out.write(FlightRecorderFileMagic, strlen(FlightRecorderFileMagic)+1);
    PropertiesWrite(config, out);
    
    if (tape_type == FGTapeType_CONTINUOUS) {
        // Ensure that all recorded properties are written in first frame.
        //
        m_pRecorder->resetExtraProperties();
    }
    
    if (!out) {
        out.close();
        config = nullptr;
    }
    return config;
}

static void writeFrame2(FGReplayData* r, std::ostream& out, SGPropertyNode_ptr config)
{
    for (auto data: config->getChildren("data")) {
        const char* data_type = data->getStringValue();
        if (!strcmp(data_type, "signals")) {
            uint32_t    signals_size = r->raw_data.size();
            out.write(reinterpret_cast<char*>(&signals_size), sizeof(signals_size));
            out.write(&r->raw_data.front(), r->raw_data.size());
        }
        else if (!strcmp(data_type, "multiplayer")) {
            uint32_t    length = 0;
            for (auto message: r->multiplayer_messages) {
                length += sizeof(uint16_t) + message->size();
            }
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "data_type=" << data_type << " out.tellp()=" << out.tellp()
                    << " length=" << length);
            out.write(reinterpret_cast<const char*>(&length), sizeof(length));
            for (auto message: r->multiplayer_messages) {
                uint16_t message_size = message->size();
                out.write(reinterpret_cast<const char*>(&message_size), sizeof(message_size));
                out.write(&message->front(), message_size);
            }
        }
        else if (!strcmp(data_type, "extra-properties")) {
            uint32_t    length = r->extra_properties.size();
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "data_type=" << data_type << " out.tellp()=" << out.tellp()
                    << " length=" << length);
            out.write(reinterpret_cast<char*>(&length), sizeof(length));
            out.write(&r->extra_properties[0], length);
        }
        else {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "unrecognised data_type=" << data_type);
            assert(0);
        }
    }
    
}

// Writes one frame of continuous record information.
//
bool
FGReplay::continuousWriteFrame(FGReplayData* r, std::ostream& out, SGPropertyNode_ptr config)
{
    SG_LOG(SG_SYSTEMS, SG_BULK, "writing frame."
            << " out.tellp()=" << out.tellp()
            << " r->sim_time=" << r->sim_time
            );
    // Don't write frame if no data to write.
    //bool r_has_data = false;
    bool has_signals = false;
    bool has_multiplayer = false;
    bool has_extra_properties = false;
    for (auto data: config->getChildren("data")) {
        const char* data_type = data->getStringValue();
        if (!strcmp(data_type, "signals")) {
            has_signals = true;
        }
        else if (!strcmp(data_type, "multiplayer")) {
            if (!r->multiplayer_messages.empty()) {
                has_multiplayer = true;
            }
        }
        else if (!strcmp(data_type, "extra-properties")) {
            if (!r->extra_properties.empty()) {
                has_extra_properties = true;
            }
        }
        else {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "unrecognised data_type=" << data_type);
            assert(0);
        }
    }
    if (!has_signals && !has_multiplayer && !has_extra_properties) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Not writing frame because no data to write");
        return true;
    }
    
    out.write(reinterpret_cast<char*>(&r->sim_time), sizeof(r->sim_time));
    
    if (m_continuous_out_compression) {
        uint8_t flags = 0;
        if (has_signals)            flags |= 1;
        if (has_multiplayer)        flags |= 2;
        if (has_extra_properties)   flags |= 4;
        out.write((char*) &flags, sizeof(flags));
        
        /* We need to first write the size of the compressed data so compress
        to a temporary ostringstream first. */
        std::ostringstream  compressed;
        compression_ostream out_compressing(compressed, 1024, 1024);
        writeFrame2(r, out_compressing, config);
        out_compressing.flush();
        
        uint32_t compressed_size = compressed.str().size();
        out.write((char*) &compressed_size, sizeof(compressed_size));
        out.write((char*) compressed.str().c_str(), compressed.str().size());
    }
    else
    {
        writeFrame2(r, out, config);
    }
    bool ok = true;
    if (!out) ok = false;
    return ok;
}

struct FGFrameInfo
{
    size_t  offset;
    bool    has_signals = false;
    bool    has_multiplayer = false;
    bool    has_extra_properties = false;
};

std::ostream& operator << (std::ostream& out, const FGFrameInfo& frame_info)
{
    return out << "{"
            << " offset=" << frame_info.offset
            << " has_multiplayer=" << frame_info.has_multiplayer
            << " has_extra_properties=" << frame_info.has_extra_properties
            << "}";
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
                m_continuous_in_time_to_frameinfo.clear();
            }
            assert(m_continuous_in_time_to_frameinfo.empty());

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

            current_time += dt;
            SG_LOG(SG_GENERAL, SG_BULK, "current_time=" << std::fixed << std::setprecision(6) << current_time);
            
            bool IsFinished = replay( current_time );
            if (IsFinished)
            {
                if (!was_finished_already)
                {
                    replay_master_eof->setIntValue(1);
                    guiMessage("End of tape. 'Esc' to return.");
                    was_finished_already = true;
                }
                current_time = (replay_looped->getBoolValue()) ? -1 : get_end_time()+0.01;
            }
            else
            {
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

    if (m_simple_time_enabled->getBoolValue())
    {
        sim_time = globals->get_subsystem<TimeManager>()->getMPProtocolClockSec();
    }
    else
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
    
    m_record_normal_end->setDoubleValue(r->sim_time);
    if (m_record_normal_begin->getDoubleValue() == 0)
    {
        m_record_normal_begin->setDoubleValue(r->sim_time);
    }

    if (!st_front)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "ReplaySystem: Inconsistent data!");
    }
    
    if (m_continuous_out.is_open()) {
        continuousWriteFrame(r, m_continuous_out, m_continuous_out_config);
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
            
            // Write recovery recording periodically.
            //
            if (t - s_last_recovery >= recovery_period_s) {
                s_last_recovery = t;

                // We use static variables here to avoid calculating the same
                // data each time we are called.
                //
                static SGPath               path = makeSavePath(FGTapeType_RECOVERY);
                static SGPath               path_temp = SGPath( path.str() + "-");
                //static SGPropertyNode_ptr   MetaData;
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
                SGPropertyNode_ptr config = continuousWriteHeader(out, path_temp, FGTapeType_RECOVERY);
                if (!config) ok = false;
                if (ok) ok = continuousWriteFrame(r, out, config);
                out.close();
                if (ok) {
                    rename(path_temp.c_str(), path.c_str());
                }
                else {
                    std::string message = "Failed to update recovery snapshot file '" + path.str() + "';"
                            + " See File / Flight Recorder Control / 'Maintain recovery snapshot'."
                            ;
                    popupTip(message.c_str(), 10 /*delay*/);
                }
            }
        }
    }

    if ( sim_time - st_front->sim_time > m_high_res_time )
    {
        while ( !short_term.empty() && sim_time - st_front->sim_time > m_high_res_time )
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
            if (!short_term.empty()) {
                st_front = short_term.front();
                medium_term.push_back( st_front );
                short_term.pop_front();
            }

            if (!medium_term.empty())
            {
                FGReplayData *mt_front = medium_term.front();
                if ( sim_time - mt_front->sim_time > m_medium_res_time )
                {
                    while ( !medium_term.empty() && sim_time - mt_front->sim_time > m_medium_res_time )
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
                        if (!medium_term.empty()) {
                            mt_front = medium_term.front();
                            long_term.push_back( mt_front );
                            medium_term.pop_front();
                        }

                        if (!long_term.empty())
                        {
                            FGReplayData *lt_front = long_term.front();
                            if ( sim_time - lt_front->sim_time > m_low_res_time )
                            {
                                while ( !long_term.empty() && sim_time - lt_front->sim_time > m_low_res_time )
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
FGReplay::record(double sim_time)
{
    FGReplayData* r = NULL;

    if (! recycler.empty())
    {
        r = recycler.front();
        recycler.pop_front();
    }

    return m_pRecorder->capture(sim_time, r);
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
    
    std::lock_guard<std::mutex> lock(m_continuous_in_time_to_frameinfo_lock);
    
    if (!m_continuous_in_time_to_frameinfo.empty()) {
        // We are replaying a continuous recording.
        //
        
        // We need to detect whether replay() updates the values for the main
        // window's position and size.
        int xpos0 = m_sim_startup_xpos->getIntValue();
        int ypos0 = m_sim_startup_xpos->getIntValue();
        int xsize0 = m_sim_startup_xpos->getIntValue();
        int ysize0 = m_sim_startup_xpos->getIntValue();
        
        int xpos = xpos0;
        int ypos = ypos0;
        int xsize = xsize0;
        int ysize = ysize0;
        
        double  multiplayer_recent = 3;
        
        // We replay all frames from just after the previously-replayed frame,
        // in order to replay extra properties and multiplayer aircraft
        // correctly.
        //
        double  t_begin = m_continuous_in_frame_time_last;
        
        if (time < m_continuous_in_time_last) {
            // We have gone backwards, e.g. user has clicked on the back
            // buttons in the Replay dialogue.
            //
            
            if (m_continuous_in_multiplayer) {
                // Continuous recording has multiplayer data, so replay recent
                // ones.
                //
                t_begin = time - multiplayer_recent;
            }
            
            if (m_continuous_in_extra_properties) {
                // Continuous recording has property changes. we need to replay
                // all property changes from the beginning.
                //
                t_begin = -1;
            }
            
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "Have gone backwards."
                    << " m_continuous_in_time_last=" << m_continuous_in_time_last
                    << " time=" << time
                    << " t_begin=" << t_begin
                    << " m_continuous_in_extra_properties=" << m_continuous_in_extra_properties
                    );
        }
        
        // Prepare to replay signals from uncompressed recording file. We want
        // to find a pair of frames that straddle the requested <time> so that
        // we can interpolate.
        //
        auto p = m_continuous_in_time_to_frameinfo.lower_bound(time);
        bool ret = false;
        
        size_t  offset;
        size_t  offset_prev = 0;
        
        if (p == m_continuous_in_time_to_frameinfo.end()) {
            // We are at end of recording; replay last frame.
            --p;
            offset = p->second.offset;
            ret = true;
        }
        else if (p->first > time) {
            // Look for preceding item.
            if (p == m_continuous_in_time_to_frameinfo.begin()) {
                // <time> is before beginning of recording.
                offset = p->second.offset;
            }
            else {
                // Interpolate between pair of items that straddle <time>.
                auto prev = p;
                --prev;
                offset_prev = prev->second.offset;
                offset = p->second.offset;
            }
        }
        else {
            // Exact match.
            offset = p->second.offset;
        }
        
        // Before interpolating signals, we replay all property changes from
        // all frame times t satisfying t_prop_begin < t < time. We also replay
        // all recent multiplayer packets in this range, i.e. for which t >
        // time - multiplayer_recent.
        //
        for (auto p_before = m_continuous_in_time_to_frameinfo.upper_bound(t_begin);
                p_before != m_continuous_in_time_to_frameinfo.end();
                ++p_before) {
            if (p_before->first >= p->first) {
                break;
            }
            // Replaying a frame is expensive because we read frame data
            // from disc each time. So we only replay this frame if it has
            // extra_properties, or if it has multiplayer packets and we are
            // within <multiplayer_recent> seconds of current time.
            //
            bool    replay_this_frame = p_before->second.has_extra_properties;
            if (p_before->second.has_multiplayer && p_before->first > time - multiplayer_recent) {
                replay_this_frame = true;
            }
            
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "Looking at extra property changes."
                    << " replay_this_frame=" << replay_this_frame
                    << " m_continuous_in_time_last=" << m_continuous_in_time_last
                    << " m_continuous_in_frame_time_last=" << m_continuous_in_frame_time_last
                    << " time=" << time
                    << " t_begin=" << t_begin
                    << " p_before->first=" << p_before->first
                    << " p_before->second=" << p_before->second
                    );

            if (replay_this_frame) {
                replay(
                        p_before->first,
                        p_before->second.offset,
                        0 /*offset_old*/,
                        false /*replay_signals*/,
                        p_before->first > time - multiplayer_recent /*replay_multiplayer*/,
                        true /*replay_extra_properties*/,
                        &xpos,
                        &ypos,
                        &xsize,
                        &ysize
                        );
            }
        }
        
        /* Now replay signals, interpolating between frames atoffset_prev and
        offset. */
        replay(
                time,
                offset,
                offset_prev /*offset_old*/,
                true /*replay_signals*/,
                true /*replay_multiplayer*/,
                true /*replay_extra_properties*/,
                &xpos,
                &ypos,
                &xsize,
                &ysize
                );
        
        if (0
                || xpos != xpos0
                || ypos != ypos0
                || xsize != xsize0
                || ysize != ysize0
                ) {
            // Move/resize the main window to reflect the updated values.
            globals->get_props()->setIntValue("/sim/startup/xpos", xpos);
            globals->get_props()->setIntValue("/sim/startup/ypos", ypos);
            globals->get_props()->setIntValue("/sim/startup/xsize", xsize);
            globals->get_props()->setIntValue("/sim/startup/ysize", ysize);
            
            osgViewer::ViewerBase* viewer_base = globals->get_renderer()->getViewerBase();
            if (viewer_base) {
                std::vector<osgViewer::GraphicsWindow*> windows;
                viewer_base->getWindows(windows);
                osgViewer::GraphicsWindow* window = windows[0];

                // We use FGEventHandler::setWindowRectangle() to move the
                // window, because it knows how to convert from window work-area
                // coordinates to window-including-furniture coordinates.
                //
                flightgear::FGEventHandler* event_handler = globals->get_renderer()->getEventHandler();
                event_handler->setWindowRectangleInteriorWithCorrection(window, xpos, ypos, xsize, ysize);
            }
        }
        
        m_continuous_in_time_last = time;
        m_continuous_in_frame_time_last = p->first;
        
        return ret;
    }

    if ( ! short_term.empty() ) {
        // We use /sim/replay/end-time instead of short_term.back()->sim_time
        // because if we are recording multiplayer aircraft, new items will
        // be appended to short_term while we replay, and we don't want to
        // continue replaying into them. /sim/replay/end-time will remain
        // pointing to the last frame at the time we started replaying.
        //
        t1 = fgGetDouble("/sim/replay/end-time");
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
FGReplay::replay(double time, FGReplayData* pCurrentFrame, FGReplayData* pOldFrame,
        int* xpos, int* ypos, int* xsize, int* ysize)
{
    m_pRecorder->replay(time, pCurrentFrame, pOldFrame, xpos, ypos, xsize, ysize);
}

static int16_t read_int16(std::istream& in, size_t& pos)
{
    int16_t a;
    in.read(reinterpret_cast<char*>(&a), sizeof(a));
    pos += sizeof(a);
    return a;
}
static std::string read_string(std::istream& in, size_t& pos)
{
    int16_t length = read_int16(in, pos);
    std::vector<char>   path(length);
    in.read(&path[0], length);
    pos += length;
    std::string ret(&path[0], length);
    return ret;
}

/* Reads extra-property change items in next <length> bytes. Throws if we don't
exactly read <length> bytes. */
static void ReadFGReplayDataExtraProperties(std::istream& in, FGReplayData* replay_data, uint32_t length)
{
    SG_LOG(SG_SYSTEMS, SG_BULK, "reading extra-properties. length=" << length);
    size_t pos=0;
    for(;;) {
        if (pos == length) {
            break;
        }
        if (pos > length) {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Overrun while reading extra-properties:"
                    " length=" << length << ": pos=" << pos);
            assert(0);
        }
        SG_LOG(SG_SYSTEMS, SG_BULK, "length=" << length<< " pos=" << pos);
        std::string path = read_string(in, pos);
        if (path == "") {
            path = read_string(in, pos);
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "property deleted: " << path);
            replay_data->replay_extra_property_removals.push_back(path);
        }
        else {
            std::string value = read_string(in, pos);
            SG_LOG(SG_SYSTEMS, SG_DEBUG, "property changed: " << path << "=" << value);
            replay_data->replay_extra_property_changes[path] = value;
        }
    }
}

static bool ReadFGReplayData2(
        std::istream& in,
        SGPropertyNode* config,
        bool load_signals,
        bool load_multiplayer,
        bool load_extra_properties,
        FGReplayData* ret
        )
{
    ret->raw_data.resize(0);
    for (auto data: config->getChildren("data")) {
        const char* data_type = data->getStringValue();
        SG_LOG(SG_SYSTEMS, SG_BULK, "in.tellg()=" << in.tellg() << " data_type=" << data_type);
        uint32_t    length;
        in.read(reinterpret_cast<char*>(&length), sizeof(length));
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "length=" << length);
        if (!in) break;
        if (load_signals && !strcmp(data_type, "signals")) {
            ret->raw_data.resize(length);
            in.read(&ret->raw_data.front(), ret->raw_data.size());
        }
        else if (load_multiplayer && !strcmp(data_type, "multiplayer")) {
            /* Multiplayer information is a vector of vectors. */
            ret->multiplayer_messages.clear();
            uint32_t pos = 0;
            for(;;) {
                assert(pos <= length);
                if (pos == length) {
                    break;
                }
                std::shared_ptr<std::vector<char>>  v(new std::vector<char>);
                ret->multiplayer_messages.push_back(v);
                pos += VectorRead<uint16_t>(in, *ret->multiplayer_messages.back(), length - pos);
                SG_LOG(SG_SYSTEMS, SG_BULK, "replaying multiplayer data"
                        << " ret->sim_time=" << ret->sim_time
                        << " length=" << length
                        << " pos=" << pos
                        << " callsign=" << ((T_MsgHdr*) &v->front())->Callsign
                        );
            }
        }
        else if (load_extra_properties && !strcmp(data_type, "extra-properties")) {
            ReadFGReplayDataExtraProperties(in, ret, length);
        }
        else {
            SG_LOG(SG_GENERAL, SG_BULK, "Skipping unrecognised/unwanted data: " << data_type);
            in.seekg(length, std::ios_base::cur);
        }
    }
    if (!in) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Failed to read fgtape data");
        return false;
    }
    return true;
}

/* Reads all or part of a FGReplayData from uncompressed file. */
static std::unique_ptr<FGReplayData> ReadFGReplayData(
        std::ifstream& in,
        size_t pos,
        SGPropertyNode* config,
        bool load_signals,
        bool load_multiplayer,
        bool load_extra_properties,
        int m_continuous_in_compression
        )
{
    /* Need to clear any eof bit, otherwise seekg() will not work (which is
    pretty unhelpful). E.g. see:
        https://stackoverflow.com/questions/16364301/whats-wrong-with-the-ifstream-seekg
    */
    SG_LOG(SG_SYSTEMS, SG_BULK, "reading frame. pos=" << pos);
    in.clear();
    in.seekg(pos);
    
    std::unique_ptr<FGReplayData>   ret(new FGReplayData);

    in.read(reinterpret_cast<char*>(&ret->sim_time), sizeof(ret->sim_time));
    if (!in) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Failed to read fgtape frame at offset " << pos);
        return nullptr;
    }
    bool ok;
    if (m_continuous_in_compression)
    {
        uint8_t     flags;
        uint32_t    compressed_size;
        in.read((char*) &flags, sizeof(flags));
        in.read((char*) &compressed_size, sizeof(compressed_size));
        simgear::ZlibDecompressorIStream    in_decompress(in, SGPath(), simgear::ZLibCompressionFormat::ZLIB_RAW);
        ok = ReadFGReplayData2(in_decompress, config, load_signals, load_multiplayer, load_extra_properties, ret.get());
    }
    else
    {
        ok = ReadFGReplayData2(in, config, load_signals, load_multiplayer, load_extra_properties, ret.get());
    }
    if (!ok) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Failed to read fgtape frame at offset " << pos);
        return nullptr;
    }
    return ret;
}

// Replays one frame from uncompressed file. <offset> and <offset_old>
// are offsets in file of frames that are >= and < <time> respectively.
// <offset_old> may be 0, in which case it is ignored.
//
// We load the frame(s) from disc, omitting some data depending on
// replay_signals, replay_multiplayer and replay_extra_properties. Then call
// m_pRecorder->replay(), which updates the global state.
//
void FGReplay::replay(
        double time,
        size_t offset,
        size_t offset_old,
        bool replay_signals,
        bool replay_multiplayer,
        bool replay_extra_properties,
        int* xpos,
        int* ypos,
        int* xsize,
        int* ysize
        )
{
    std::unique_ptr<FGReplayData> replay_data = ReadFGReplayData(
            m_continuous_in,
            offset,
            m_continuous_in_config,
            replay_signals,
            replay_multiplayer,
            replay_extra_properties,
            m_continuous_in_compression
            );
    if (!replay_data) {
        if (!replay_error->getBoolValue()) {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to read fgtape frame at offset=" << offset << " time=" << time);
            popupTip("Replay failed: cannot read fgtape data", 10);
            replay_error->setBoolValue(true);
        }
        return;
    }
    assert(replay_data.get());
    std::unique_ptr<FGReplayData> replay_data_old;
    if (offset_old) {
        replay_data_old = ReadFGReplayData(
                m_continuous_in,
                offset_old,
                m_continuous_in_config,
                replay_signals,
                replay_multiplayer,
                replay_extra_properties,
                m_continuous_in_compression
                );
    }
    if (replay_extra_properties) SG_LOG(SG_SYSTEMS, SG_BULK,
            "FGReplay::replay():"
            << " time=" << time
            << " offset=" << offset
            << " offset_old=" << offset_old
            << " replay_data->raw_data.size()=" << replay_data->raw_data.size()
            << " replay_data->multiplayer_messages.size()=" << replay_data->multiplayer_messages.size()
            << " replay_data->extra_properties.size()=" << replay_data->extra_properties.size()
            << " replay_data->replay_extra_property_changes.size()=" << replay_data->replay_extra_property_changes.size()
            );
    m_pRecorder->replay(time, replay_data.get(), replay_data_old.get(), xpos, ypos, xsize, ysize);
}

double
FGReplay::get_start_time()
{
    double ret;
    std::lock_guard<std::mutex> lock(m_continuous_in_time_to_frameinfo_lock);
    if (!m_continuous_in_time_to_frameinfo.empty()) {
        ret = m_continuous_in_time_to_frameinfo.begin()->first;
        SG_LOG(SG_SYSTEMS, SG_DEBUG,
                "ret=" << ret
                << " m_continuous_in_time_to_frameinfo is "
                << m_continuous_in_time_to_frameinfo.begin()->first
                << ".."
                << m_continuous_in_time_to_frameinfo.rbegin()->first
                );
        // We don't set /sim/replay/end-time here - it is updated when indexing
        // in the background.
        return ret;
    }
    
    if ( ! long_term.empty() )
    {
        ret = long_term.front()->sim_time;
    } else if ( ! medium_term.empty() )
    {
        ret = medium_term.front()->sim_time;
    } else if ( ! short_term.empty() )
    {
        ret = short_term.front()->sim_time;
    } else
    {
        ret = 0.0;
    }
    fgSetDouble("/sim/replay/start-time", ret);
    setTimeStr("/sim/replay/start-time-str", ret);
    return ret;
}

double
FGReplay::get_end_time()
{
    std::lock_guard<std::mutex> lock(m_continuous_in_time_to_frameinfo_lock);
    if (!m_continuous_in_time_to_frameinfo.empty()) {
        double ret = m_continuous_in_time_to_frameinfo.rbegin()->first;
        SG_LOG(SG_SYSTEMS, SG_DEBUG,
                "ret=" << ret
                << " m_continuous_in_time_to_frameinfo is "
                << m_continuous_in_time_to_frameinfo.begin()->first
                << ".."
                << m_continuous_in_time_to_frameinfo.rbegin()->first
                );
        // We don't set /sim/replay/end-time here - it is updated when indexing
        // in the background.
        return ret;
    }
    double ret = short_term.empty() ? 0 : short_term.back()->sim_time;
    fgSetDouble("/sim/replay/end-time", ret);
    setTimeStr("/sim/replay/end-time-str", ret);
    return ret;
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
            uint32_t    length;
            input.read(reinterpret_cast<char*>(&length), sizeof(length));
            uint32_t    pos = 0;
            for(;;) {
                if (pos == length) break;
                assert(pos < length);
                uint16_t message_size;
                input.read(reinterpret_cast<char*>(&message_size), sizeof(message_size));
                pos += sizeof(message_size) + message_size;
                if (pos > length) {
                    SG_LOG(SG_SYSTEMS, SG_ALERT, "Tape appears to have corrupted multiplayer data");
                    return false;
                }
                auto message = std::make_shared<std::vector<char>>(message_size);
                //std::shared_ptr<std::vector<char>>  message(new std::vector<char>(message_size));
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
        //bool multiplayer = MetaData->getBoolValue("meta/multiplayer", 0);

        if (ok)
            ok &= saveRawReplayData(output, short_term,  RecordSize, MetaData);
        if (ok)
            ok &= saveRawReplayData(output, medium_term, RecordSize, MetaData);
        if (ok)
            ok &= saveRawReplayData(output, long_term,   RecordSize, MetaData);
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
    SGPath path_timeless;
    SGPath path = makeSavePath(FGTapeType_NORMAL, &path_timeless);
    SGPropertyNode_ptr MetaData = saveSetup(Extra, path, get_end_time()-get_start_time(), FGTapeType_NORMAL);
    bool ok = false;
    if (MetaData) {
        ok = saveTape(path, MetaData);
        if (ok) {
            // Make a convenience link to the recording. E.g.
            // harrier-gr3.fgtape ->
            // harrier-gr3-20201224-005034.fgtape.
            //
            // Link destination is in same directory as link so we use leafname
            // path.file().
            //
            path_timeless.remove();
            ok = path_timeless.makeLink(path.file());
            if (!ok) {
                SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to create link " << path_timeless.c_str() << " => " << path.file());
            }
        }
    }
    if (ok)
        guiMessage("Flight recorder tape saved successfully!");
    else
        guiMessage("Failed to save tape! See log output.");

    return ok;
}


int FGReplay::loadContinuousHeader(const std::string& path, std::istream* in, SGPropertyNode* properties)
{
    std::ifstream   in0;
    if (!in) {
        in0.open(path);
        in = &in0;
    }
    if (!*in) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Failed to open path=" << path);
        return +1;
    }
    std::vector<char>   buffer(strlen( FlightRecorderFileMagic) + 1);
    in->read(&buffer.front(), buffer.size());
    SG_LOG(SG_SYSTEMS, SG_DEBUG, "in->gcount()=" << in->gcount() << " buffer.size()=" << buffer.size());
    if ((size_t) in->gcount() != buffer.size()) {
        // Further download is needed.
        return +1;
    }
    if (strcmp(&buffer.front(), FlightRecorderFileMagic)) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "fgtape prefix doesn't match FlightRecorderFileMagic in path: " << path);
        return -1;
    }
    bool ok = false;
    try {
        PropertiesRead(*in, properties);
        ok = true;
    }
    catch (std::exception& e) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Failed to read Config properties in: " << path << ": " << e.what());
    }
    if (!ok) {
        // Failed to read properties, so indicate that further download is needed.
        return +1;
    }
    SG_LOG(SG_SYSTEMS, SG_BULK, "properties is:\n"
            << writePropertiesInline(properties, true /*write_all*/) << "\n");
    return 0;
}

void FGReplay::indexContinuousRecording(const void* data, size_t numbytes)
{
    SG_LOG(SG_SYSTEMS, SG_DEBUG, "Indexing Continuous recording "
            << " data=" << data
            << " numbytes=" << numbytes
            << " m_continuous_indexing_pos=" << m_continuous_indexing_pos
            << " m_continuous_in_compression=" << m_continuous_in_compression
            << " m_continuous_in_time_to_frameinfo.size()=" << m_continuous_in_time_to_frameinfo.size()
            );
    time_t t0 = time(NULL);
    std::streampos original_pos = m_continuous_indexing_pos;
    size_t original_num_frames = m_continuous_in_time_to_frameinfo.size();
    
    // Reset any EOF because there might be new data.
    m_continuous_indexing_in.clear();
    
    struct data_stats_t
    {
        size_t  num_frames = 0;
        size_t  bytes = 0;
    };
    std::map<std::string, data_stats_t> stats;
    
    for(;;)
    {
        SG_LOG(SG_SYSTEMS, SG_BULK, "reading frame."
                << " m_continuous_in.tellg()=" << m_continuous_in.tellg()
                );
        m_continuous_indexing_in.seekg(m_continuous_indexing_pos);
        double sim_time;
        m_continuous_indexing_in.read(reinterpret_cast<char*>(&sim_time), sizeof(sim_time));

        SG_LOG(SG_SYSTEMS, SG_BULK, ""
                << " m_continuous_indexing_pos=" << m_continuous_indexing_pos
                << " m_continuous_indexing_in.tellg()=" << m_continuous_indexing_in.tellg()
                << " sim_time=" << sim_time
                );
        
        FGFrameInfo frameinfo;
        frameinfo.offset = m_continuous_indexing_pos;
        if (m_continuous_in_compression)
        {
            // Skip compressed frame data without decompressing it.
            uint8_t    flags;
            m_continuous_indexing_in.read((char*) &flags, sizeof(flags));
            frameinfo.has_signals = flags & 1;
            frameinfo.has_multiplayer = flags & 2;
            frameinfo.has_extra_properties = flags & 4;
            
            if (frameinfo.has_signals)
            {
                stats["signals"].num_frames += 1;
            }
            if (frameinfo.has_multiplayer)
            {
                stats["multiplayer"].num_frames += 1;
                ++m_num_frames_multiplayer;
                m_continuous_in_multiplayer = true;
            }
            if (frameinfo.has_extra_properties)
            {
                stats["extra-properties"].num_frames += 1;
                ++m_num_frames_extra_properties;
                m_continuous_in_extra_properties = true;
            }
            
            uint32_t    compressed_size;
            m_continuous_indexing_in.read((char*) &compressed_size, sizeof(compressed_size));
            SG_LOG(SG_SYSTEMS, SG_BULK, "compressed_size=" << compressed_size);
            
            m_continuous_indexing_in.seekg(compressed_size, std::ios_base::cur);
        }
        else
        {
            // Skip frame data.
            auto datas = m_continuous_in_config->getChildren("data");
            SG_LOG(SG_SYSTEMS, SG_BULK, "datas.size()=" << datas.size());
            for (auto data: datas) {
                uint32_t    length;
                m_continuous_indexing_in.read(reinterpret_cast<char*>(&length), sizeof(length));
                SG_LOG(SG_SYSTEMS, SG_BULK,
                        "m_continuous_in.tellg()=" << m_continuous_indexing_in.tellg()
                        << " Skipping data_type=" << data->getStringValue()
                        << " length=" << length
                        );
                // Move forward <length> bytes.
                m_continuous_indexing_in.seekg(length, std::ios_base::cur);
                if (!m_continuous_indexing_in) {
                    // Dont add bogus info to <stats>.
                    break;
                }
                if (length) {
                    stats[data->getStringValue()].num_frames += 1;
                    stats[data->getStringValue()].bytes += length;
                    if (!strcmp(data->getStringValue(), "signals")) {
                        frameinfo.has_signals = true;
                    }
                    else if (!strcmp(data->getStringValue(), "multiplayer")) {
                        frameinfo.has_multiplayer = true;
                        ++m_num_frames_multiplayer;
                        m_continuous_in_multiplayer = true;
                    }
                    else if (!strcmp(data->getStringValue(), "extra-properties")) {
                        frameinfo.has_extra_properties = true;
                        ++m_num_frames_extra_properties;
                        m_continuous_in_extra_properties = true;
                    }
                }
            }
        }
        SG_LOG(SG_SYSTEMS, SG_BULK, ""
                << " pos=" << m_continuous_indexing_pos
                << " sim_time=" << sim_time
                << " m_num_frames_multiplayer=" << m_num_frames_multiplayer
                << " m_num_frames_extra_properties=" << m_num_frames_extra_properties
                );

        if (!m_continuous_indexing_in) {
            // Failed to read a frame, e.g. because of EOF. Leave
            // m_continuous_indexing_pos unchanged so we can try again at same
            // starting position if recording is upated by background download.
            //
            SG_LOG(SG_SYSTEMS, SG_BULK, "m_continuous_indexing_in failed, giving up");
            break;
        }
        
        // We have successfully read a frame, so add it to
        // m_continuous_in_time_to_frameinfo[].
        //
        m_continuous_indexing_pos = m_continuous_indexing_in.tellg();
        std::lock_guard<std::mutex> lock(m_continuous_in_time_to_frameinfo_lock);
        m_continuous_in_time_to_frameinfo[sim_time] = frameinfo;
    }
    time_t t = time(NULL) - t0;
    auto new_bytes = m_continuous_indexing_pos - original_pos;
    auto num_frames = m_continuous_in_time_to_frameinfo.size();
    auto num_new_frames = num_frames - original_num_frames;
    if (num_new_frames) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Continuous recording: index updated:"
                << " num_frames=" << num_frames
                << " num_new_frames=" << num_new_frames
                << " new_bytes=" << new_bytes
                );
    }
    SG_LOG(SG_SYSTEMS, SG_DEBUG, "Continuous recording indexing complete."
            << " time taken=" << t << "s."
            << " num_new_frames=" << num_new_frames
            << " m_continuous_indexing_pos=" << m_continuous_indexing_pos
            << " m_continuous_in_time_to_frameinfo.size()=" << m_continuous_in_time_to_frameinfo.size()
            << " m_num_frames_multiplayer=" << m_num_frames_multiplayer
            << " m_num_frames_extra_properties=" << m_num_frames_extra_properties
            );
    for (auto stat: stats) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "data type " << stat.first << ":"
                << " num_frames=" << stat.second.num_frames
                << " bytes=" << stat.second.bytes
                );
    }
    
    std::lock_guard<std::mutex> lock(m_continuous_in_time_to_frameinfo_lock);
    fgSetInt("/sim/replay/continuous-stats-num-frames", m_continuous_in_time_to_frameinfo.size());
    fgSetInt("/sim/replay/continuous-stats-num-frames-extra-properties", m_num_frames_extra_properties);
    fgSetInt("/sim/replay/continuous-stats-num-frames-multiplayer", m_num_frames_multiplayer);
    if (!m_continuous_in_time_to_frameinfo.empty()) {
        double t_begin = m_continuous_in_time_to_frameinfo.begin()->first;
        double t_end = m_continuous_in_time_to_frameinfo.rbegin()->first;
        fgSetDouble("/sim/replay/start-time", t_begin);
        fgSetDouble("/sim/replay/end-time", t_end);
        setTimeStr("/sim/replay/start-time-str", t_begin);
        setTimeStr("/sim/replay/end-time-str", t_end);
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Have set /sim/replay/end-time to " << fgGetDouble("/sim/replay/end-time"));
    }
    if (!numbytes) {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Continuous recording: indexing finished"
                << " m_continuous_in_time_to_frameinfo.size()=" << m_continuous_in_time_to_frameinfo.size()
                );
        m_continuous_indexing_in.close();
    }
}

void FGReplay::call_indexContinuousRecording(void* ref, const void* data, size_t numbytes)
{
    FGReplay* self = (FGReplay*) ref;
    SG_LOG(SG_GENERAL, SG_BULK, "calling indexContinuousRecording() data=" << data << " numbytes=" << numbytes);
    self->indexContinuousRecording(data, numbytes);
}

/** Read a flight recorder tape with given filename from disk.
 * Copies MetaData's "meta" node into MetaMeta out-param.
 * Actual data and signal configuration is not read when in "Preview" mode.
 */
bool
FGReplay::loadTape(const SGPath& Filename, bool Preview, SGPropertyNode& MetaMeta, simgear::HTTP::FileRequestRef filerequest)
{
    SG_LOG(SG_SYSTEMS, SG_DEBUG, "loading Preview=" << Preview << " Filename=" << Filename);

    /* Try to load a Continuous recording first. */
    replay_error->setBoolValue(false);
    std::ifstream   in_preview;
    std::ifstream&  in(Preview ? in_preview : m_continuous_in);
    in.open(Filename.str());
    if (!in) {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "Failed to open"
                << " Filename=" << Filename.str()
                << " in.is_open()=" << in.is_open()
                );
        return false;
    }
    m_continuous_in_config = new SGPropertyNode;
    int e = loadContinuousHeader(Filename.str(), &in, m_continuous_in_config);
    if (e == 0) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "m_continuous_in_config is:\n"
                << writePropertiesInline(m_continuous_in_config, true /*write_all*/) << "\n");
        copyProperties(m_continuous_in_config->getNode("meta", 0, true), &MetaMeta);
        if (Preview) {
            in.close();
            return true;
        }
        m_pRecorder->reinit(m_continuous_in_config);
        clear();
        fillRecycler();
        m_continuous_in_time_last = -1;
        m_continuous_in_frame_time_last = -1;
        m_continuous_in_time_to_frameinfo.clear();
        m_num_frames_extra_properties = 0;
        m_num_frames_multiplayer = 0;
        m_continuous_indexing_in.open(Filename.str());
        m_continuous_indexing_pos = in.tellg();
        m_continuous_in_compression = m_continuous_in_config->getNode("meta/continuous-compression", true /*create*/)->getIntValue();
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "m_continuous_in_compression=" << m_continuous_in_compression);
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "filerequest=" << filerequest.get());

        // Make an in-memory index of the recording.
        if (filerequest) {
            filerequest->setCallback(
                    [this](const void* data, size_t numbytes)
                    {
                        indexContinuousRecording(data, numbytes);
                    }
                    );
        }
        else {
            indexContinuousRecording(nullptr, 0);
        }
        start(true /*NewTape*/);
        return true;
    }
    
    // Not a continuous recording.
    in.close();
    if (filerequest) {
        SG_LOG(SG_SYSTEMS, SG_DEBUG, "Cannot load Filename=" << Filename << " because it is download but not Continuous recording");
        return false;
    }
    bool ok = true;

    /* Open as a gzipped Normal recording. ********************************************/
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

            bool multiplayer = false;
            for (auto data: MetaMeta.getChildren("data")) {
                if (!strcmp(data->getStringValue(), "multiplayer")) {
                    multiplayer = true;
                }
            }
            SG_LOG(SG_SYSTEMS, SG_ALERT, "multiplayer=" << multiplayer);
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

std::string  FGReplay::makeTapePath(const std::string& tape_name)
{
    std::string path = tape_name;
    if (simgear::strutils::ends_with(path, ".fgtape")) {
        return path;
    }
    SGPath path2(fgGetString("/sim/replay/tape-directory", ""));
    path2.append(path + ".fgtape");
    return path2.str();
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
        std::string path = makeTapePath(tape);
        SG_LOG(SG_SYSTEMS, MY_SG_DEBUG, "Checking flight recorder file " << path << ", preview: " << Preview);
        return loadTape(path, Preview, *MetaMeta);
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGReplay> registrantFGReplay;
