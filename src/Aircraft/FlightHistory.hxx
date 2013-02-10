// FlightHistory
//
// Written by James Turner, started December 2012.
//
// Copyright (C) 2012 James Turner - zakalawe (at) mac com
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

#ifndef FG_AIRCRAFT_FLIGHT_HISTORY_HXX
#define FG_AIRCRAFT_FLIGHT_HISTORY_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/SGMath.hxx>

#include <vector>
#include <memory> // for std::auto_ptr

typedef std::vector<SGGeod> SGGeodVec;

const unsigned int SAMPLE_BUCKET_WIDTH = 1024;

/**
 * record the history of the aircraft's movements, making it available
 * as a contiguous block. This can be used to show the historical flight-path
 * over a long period of time (unlike the replay system), but only a small,
 * fixed set of properties are recorded. (Positioned and orientation, but
 * not velocity, acceleration, control inputs, or so on)
 */
class FGFlightHistory : public SGSubsystem
{
public:
    FGFlightHistory();
    virtual ~FGFlightHistory();
    
    virtual void init();
    virtual void shutdown();
    virtual void reinit();
    virtual void update(double dt);
    
    /**
     * retrieve the path, collapsing segments shorter than
     * the specified minimum length
     */
    SGGeodVec pathForHistory(double minEdgeLengthM = 50.0) const;
private:
    /**
     * @class A single data sample in the history system.
     */
    class Sample
    {
    public:
        SGGeod position;
        /// heading, pitch and roll can be recorded at lower precision
        /// than a double - actually 16 bits might be sufficient
        float heading, pitch, roll;
        int simTimeMSec;
    };
    
    
    
    /**
     * Bucket is a fixed-size container of samples. This is a crude slab
     * allocation of samples, in chunks defined by the width constant above.
     * Keep in mind that even with a 1Hz sample frequency, we use less than
     * 200kbytes per hour - avoiding continous malloc traffic, or expensive
     * std::vector reallocations, is the key factor here.
     */
    class SampleBucket
    {
    public:
        Sample samples[SAMPLE_BUCKET_WIDTH];
    };
    
    double m_lastCaptureTime;
    double m_sampleInterval; ///< sample interval in seconds
/// our store of samples (in buckets). The last bucket is partially full,
/// with the number of valid samples indicated by m_validSampleCount
    std::vector<SampleBucket*> m_buckets;
    
/// number of valid samples in the final bucket
    unsigned int m_validSampleCount;
    
    SGPropertyNode_ptr m_weightOnWheels;
    SGPropertyNode_ptr m_enabled;
  
    bool m_lastWoW;
    size_t m_maxMemoryUseBytes;
  
    void allocateNewBucket();
    
    void clear();
    void capture();
  
    size_t currentMemoryUseBytes() const;
};

#endif