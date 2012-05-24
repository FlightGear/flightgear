// flightrecorder.cxx
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/ResourceManager.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGMath.hxx>
#include <Main/fg_props.hxx>
#include "flightrecorder.hxx"

using namespace FlightRecorder;

FGFlightRecorder::FGFlightRecorder(const char* pConfigName) :
    m_RecorderNode(fgGetNode("/sim/flight-recorder", true)),
    m_TotalRecordSize(0),
    m_ConfigName(pConfigName)
{
}

FGFlightRecorder::~FGFlightRecorder()
{
}

void
FGFlightRecorder::reinit(void)
{
    m_ConfigNode = 0;

    m_TotalRecordSize = 0;

    m_CaptureDouble.clear();
    m_CaptureFloat.clear();
    m_CaptureInteger.clear();
    m_CaptureInt16.clear();
    m_CaptureInt8.clear();
    m_CaptureBool.clear();

    int Selected = m_RecorderNode->getIntValue(m_ConfigName, 0);
    SG_LOG(SG_SYSTEMS, SG_INFO, "FlightRecorder: Recorder configuration #" << Selected);
    if (Selected >= 0)
        m_ConfigNode = m_RecorderNode->getChild("config", Selected);

    if (!m_ConfigNode.valid())
        initDefault();

    if (!m_ConfigNode.valid())
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: Configuration is invalid. Flight recorder disabled.");
    }
    else
    {
        // set name of active flight recorder type 
        const char* pRecorderName =
                m_ConfigNode->getStringValue("name",
                                             "aircraft-specific flight recorder");
        SG_LOG(SG_SYSTEMS, SG_INFO, "FlightRecorder: Using custom recorder configuration: " << pRecorderName);
        m_RecorderNode->setStringValue("active-config-name", pRecorderName);

        // get signals
        initSignalList("double", m_CaptureDouble,  m_ConfigNode );
        initSignalList("float",  m_CaptureFloat ,  m_ConfigNode );
        initSignalList("int",    m_CaptureInteger, m_ConfigNode );
        initSignalList("int16",  m_CaptureInt16  , m_ConfigNode );
        initSignalList("int8",   m_CaptureInt8   , m_ConfigNode );
        initSignalList("bool",   m_CaptureBool   , m_ConfigNode );
    }

    // calculate size of a single record
    m_TotalRecordSize = sizeof(double)        * 1 /* sim time */        +
                        sizeof(double)        * m_CaptureDouble.size()  +
                        sizeof(float)         * m_CaptureFloat.size()   +
                        sizeof(int)           * m_CaptureInteger.size() +
                        sizeof(short int)     * m_CaptureInt16.size()   +
                        sizeof(signed char)   * m_CaptureInt8.size()    +
                        sizeof(unsigned char) * ((m_CaptureBool.size()+7)/8); // 8 bools per byte

    // expose size of actual flight recorder record
    m_RecorderNode->setIntValue("record-size", m_TotalRecordSize);
    SG_LOG(SG_SYSTEMS, SG_INFO, "FlightRecorder: record size is " << m_TotalRecordSize << " bytes");
}

/** Check if SignalList already contains the given property */
bool
FGFlightRecorder::haveProperty(FlightRecorder::TSignalList& SignalList,SGPropertyNode* pProperty)
{
    unsigned int Count = SignalList.size();
    for (unsigned int i=0; i<Count; i++)
    {
        if (SignalList[i].Signal.get() == pProperty)
        {
            return true;
        }
    }
    return false;
}

/** Check if any signal list already contains the given property */
bool
FGFlightRecorder::haveProperty(SGPropertyNode* pProperty)
{
    if (haveProperty(m_CaptureDouble,  pProperty))
        return true;
    if (haveProperty(m_CaptureFloat,   pProperty))
        return true;
    if (haveProperty(m_CaptureInteger, pProperty))
        return true;
    if (haveProperty(m_CaptureInt16,   pProperty))
        return true;
    if (haveProperty(m_CaptureInt8,    pProperty))
        return true;
    if (haveProperty(m_CaptureBool,    pProperty))
        return true;
    return false;
}

/** Read default flight-recorder configuration.
 * Default should match properties as hard coded for versions up to FG2.4.0. */
void
FGFlightRecorder::initDefault(void)
{
    // set name of active flight recorder type
    SG_LOG(SG_SYSTEMS, SG_INFO, "FlightRecorder: No custom configuration. Loading generic default recorder.");

    const char* Path = m_RecorderNode->getStringValue("default-config",NULL);
    if (!Path)
    {
        SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: No default flight recorder specified! Check preferences.xml!");
    }
    else
    {
        SGPath path = globals->resolve_aircraft_path(Path);
        if (path.isNull())
        {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: Cannot find file '" << Path << "'.");
        }
        else
        {
            try
            {
                readProperties(path.str(), m_RecorderNode->getChild("config", 0 ,true), 0);
                m_ConfigNode = m_RecorderNode->getChild("config", 0 ,false);
            } catch (sg_io_exception &e)
            {
                SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: Error reading file '" <<
                        Path << ": " << e.getFormattedMessage());
            }
        }
    }
}

/** Read signal list below given base node.
 * Only process properties of given signal type and add all signals to the given list.
 * This method is called for all supported signal types - properties of each type are
 * kept in separate lists for efficiency reasons. */
void
FGFlightRecorder::initSignalList(const char* pSignalType, TSignalList& SignalList, SGPropertyNode_ptr BaseNode)
{
    // clear old signals
    SignalList.clear();

    processSignalList(pSignalType, SignalList, BaseNode);

    SG_LOG(SG_SYSTEMS, SG_DEBUG, "FlightRecorder: " << SignalList.size() << " signals of type " << pSignalType );
}

/** Process signal list below given base node.
 * Only process properties of given signal type and add all signals to the given list.
 * This method is called for all supported signal types - properties of each type are
 * kept in separate lists for efficiency reasons. */
void
FGFlightRecorder::processSignalList(const char* pSignalType, TSignalList& SignalList, SGPropertyNode_ptr SignalListNode,
                                    string PropPrefix, int Count)
{
    // get the list of signal sources (property paths) for this signal type
    SGPropertyNode_ptr SignalNode;
    int Index=0;

    Count = SignalListNode->getIntValue("count",Count);
    PropPrefix = simgear::strutils::strip(SignalListNode->getStringValue("prefix",PropPrefix.c_str()));
    if ((!PropPrefix.empty())&&(PropPrefix[PropPrefix.size()-1] != '/'))
        PropPrefix += "/";

    do
    {
        SignalNode = SignalListNode->getChild("signal",Index,false);
        if (SignalNode.valid()&&
            (0==strcmp(pSignalType, SignalNode->getStringValue("type","float"))))
        {
            string PropertyPath = SignalNode->getStringValue("property","");
            if (!PropertyPath.empty())
            {
                PropertyPath = PropPrefix + PropertyPath;
                const char* pInterpolation = SignalNode->getStringValue("interpolation","linear");

                // Check if current signal has a "%i" place holder. Otherwise count is 1.
                string::size_type IndexPos = PropertyPath.find("%i");
                int SignalCount = Count;
                if (IndexPos == string::npos)
                    SignalCount = 1;

                for (int IndexValue=0;IndexValue<SignalCount;IndexValue++)
                {
                    string PPath = PropertyPath;
                    if (IndexPos != string::npos)
                    {
                        char strbuf[20];
                        snprintf(strbuf, 20, "%d", IndexValue);
                        PPath = PPath.replace(IndexPos,2,strbuf);
                    }
                    TCapture Capture;
                    Capture.Signal = fgGetNode(PPath.c_str(),false);
                    if (!Capture.Signal.valid())
                    {
                        // warn user: we're maybe going to record useless data
                        // Or maybe the data is only initialized later. Warn anyway, so we can catch useless data. 
                        SG_LOG(SG_SYSTEMS, SG_INFO, "FlightRecorder: Recording non-existent property '" << PPath << "'.");
                        Capture.Signal = fgGetNode(PPath.c_str(),true);
                    }

                    if (0==strcmp(pInterpolation,"discrete"))
                        Capture.Interpolation = discrete;
                    else 
                    if ((0==strcmp(pInterpolation,"angular"))||
                        (0==strcmp(pInterpolation,"angular-rad")))
                        Capture.Interpolation = angular_rad;
                    else
                    if (0==strcmp(pInterpolation,"angular-deg"))
                        Capture.Interpolation = angular_deg;
                    else
                    if (0==strcmp(pInterpolation,"linear"))
                        Capture.Interpolation = linear;
                    else
                    {
                        SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: Unsupported interpolation type '"
                                << pInterpolation<< "' of signal '" << PPath << "'");
                        Capture.Interpolation = linear;
                    }
                    if (haveProperty(Capture.Signal))
                    {
                        SG_LOG(SG_SYSTEMS, SG_ALERT, "FlightRecorder: Property '"
                                << PPath << "' specified multiple times. Check flight recorder configuration.");
                    }
                    else
                        SignalList.push_back(Capture);
                }
            }
        }
        Index++;
    } while (SignalNode.valid());

    // allow recursive definition of signal lists
    simgear::PropertyList Nodes = SignalListNode->getChildren("signals");
    for (unsigned int i=0;i<Nodes.size();i++)
    {
        processSignalList(pSignalType, SignalList, Nodes[i], PropPrefix, Count);
    }
}

/** Get an empty container for a single capture. */
FGReplayData*
FGFlightRecorder::createEmptyRecord(void)
{
    if (!m_TotalRecordSize)
        return NULL;
    FGReplayData* p = (FGReplayData*) new unsigned char[m_TotalRecordSize];
    return p;
}

/** Free given container with capture data. */
void
FGFlightRecorder::deleteRecord(FGReplayData* pRecord)
{
    delete[] pRecord;
}

/** Capture data.
 * When pBuffer==NULL new memory is allocated.
 * If pBuffer!=NULL memory of given buffer is reused.
 */
FGReplayData*
FGFlightRecorder::capture(double SimTime, FGReplayData* pRecycledBuffer)
{
    if (!pRecycledBuffer)
    {
        pRecycledBuffer = createEmptyRecord();
        if (!pRecycledBuffer)
            return NULL;
    }
    unsigned char* pBuffer = (unsigned char*) pRecycledBuffer;

    int Offset = 0;
    pRecycledBuffer->sim_time = SimTime;
    Offset += sizeof(double);

    // 64bit aligned data first!
    {
        // capture doubles
        double* pDoubles = (double*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureDouble.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            pDoubles[i] = m_CaptureDouble[i].Signal->getDoubleValue();
        }
        Offset += SignalCount * sizeof(double);
    }
    
    // 32bit aligned data comes second...
    {
        // capture floats
        float* pFloats = (float*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureFloat.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            pFloats[i] = m_CaptureFloat[i].Signal->getFloatValue();
        }
        Offset += SignalCount * sizeof(float);
    }
    
    {
        // capture integers (32bit aligned)
        int* pInt = (int*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInteger.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            pInt[i] = m_CaptureInteger[i].Signal->getIntValue();
        }
        Offset += SignalCount * sizeof(int);
    }
    
    // 16bit aligned data is next...
    {
        // capture 16bit short integers
        short int* pShortInt = (short int*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInt16.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            pShortInt[i] = (short int) m_CaptureInt16[i].Signal->getIntValue();
        }
        Offset += SignalCount * sizeof(short int);
    }
    
    // finally: byte aligned data is last...
    {
        // capture 8bit chars
        signed char* pChar = (signed char*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInt8.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            pChar[i] = (signed char) m_CaptureInt8[i].Signal->getIntValue();
        }
        Offset += SignalCount * sizeof(signed char);
    }
    
    {
        // capture 1bit booleans (8bit aligned)
        unsigned char* pFlags = (unsigned char*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureBool.size();
        int Size = (SignalCount+7)/8;
        Offset += Size;
        memset(pFlags,0,Size);
        for (unsigned int i=0; i<SignalCount; i++)
        {
            if (m_CaptureBool[i].Signal->getBoolValue())
                pFlags[i>>3] |= 1 << (i&7);
        }
    }

    assert(Offset == m_TotalRecordSize);

    return (FGReplayData*) pBuffer;
}

/** Do interpolation as defined by given interpolation type and weighting ratio. */
static double
weighting(TInterpolation interpolation, double ratio, double v1,double v2)
{
    switch (interpolation)
    {
        case linear:
            return v1 + ratio*(v2-v1);
        
        case angular_deg:
        {
            // special handling of angular data
            double tmp = v2 - v1;
            if ( tmp > 180 )
                tmp -= 360;
            else if ( tmp < -180 )
                tmp += 360;
            return v1 + tmp * ratio;
        }

        case angular_rad:
        {
            // special handling of angular data
            double tmp = v2 - v1;
            if ( tmp > SGD_PI )
                tmp -= SGD_2PI;
            else if ( tmp < -SGD_PI )
                tmp += SGD_2PI;
            return v1 + tmp * ratio;
        }

        case discrete:
            // fall through
        default:
            return v2;
    }
}

/** Replay.
 * Restore all properties with data from given buffer. */
void
FGFlightRecorder::replay(double SimTime, const FGReplayData* _pNextBuffer, const FGReplayData* _pLastBuffer)
{
    const char* pLastBuffer = (const char*) _pLastBuffer;
    const char* pBuffer = (const char*) _pNextBuffer;
    if (!pBuffer)
        return;

    int Offset = 0;
    double ratio;
    if (pLastBuffer)
    {
        double NextSimTime = _pNextBuffer->sim_time;
        double LastSimTime = _pLastBuffer->sim_time;
        ratio = (SimTime - LastSimTime) / (NextSimTime - LastSimTime);
    }
    else
    {
        ratio = 1.0;
    }

    Offset += sizeof(double);

    // 64bit aligned data first!
    {
        // restore doubles
        const double* pDoubles = (const double*) &pBuffer[Offset];
        const double* pLastDoubles = (const double*) &pLastBuffer[Offset];
        unsigned int SignalCount = m_CaptureDouble.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            double v = pDoubles[i];
            if (pLastBuffer)
            {
                v = weighting(m_CaptureDouble[i].Interpolation, ratio,
                              pLastDoubles[i], v);
            }
            m_CaptureDouble[i].Signal->setDoubleValue(v);
        }
        Offset += SignalCount * sizeof(double);
    }

    // 32bit aligned data comes second...
    {
        // restore floats
        const float* pFloats = (const float*) &pBuffer[Offset];
        const float* pLastFloats = (const float*) &pLastBuffer[Offset];
        unsigned int SignalCount = m_CaptureFloat.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            float v = pFloats[i];
            if (pLastBuffer)
            {
                v = weighting(m_CaptureFloat[i].Interpolation, ratio,
                              pLastFloats[i], v);
            }
            m_CaptureFloat[i].Signal->setDoubleValue(v);//setFloatValue
        }
        Offset += SignalCount * sizeof(float);
    }

    {
        // restore integers (32bit aligned)
        const int* pInt = (const int*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInteger.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            m_CaptureInteger[i].Signal->setIntValue(pInt[i]);
        }
        Offset += SignalCount * sizeof(int);
    }

    // 16bit aligned data is next...
    {
        // restore 16bit short integers
        const short int* pShortInt = (const short int*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInt16.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            m_CaptureInt16[i].Signal->setIntValue(pShortInt[i]);
        }
        Offset += SignalCount * sizeof(short int);
    }

    // finally: byte aligned data is last...
    {
        // restore 8bit chars
        const signed char* pChar = (const signed char*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureInt8.size();
        for (unsigned int i=0; i<SignalCount; i++)
        {
            m_CaptureInt8[i].Signal->setIntValue(pChar[i]);
        }
        Offset += SignalCount * sizeof(signed char);
    }

    {
        // restore 1bit booleans (8bit aligned)
        const unsigned char* pFlags = (const unsigned char*) &pBuffer[Offset];
        unsigned int SignalCount = m_CaptureBool.size();
        int Size = (SignalCount+7)/8;
        Offset += Size;
        for (unsigned int i=0; i<SignalCount; i++)
        {
            m_CaptureBool[i].Signal->setBoolValue(0 != (pFlags[i>>3] & (1 << (i&7))));
        }
    }
}
