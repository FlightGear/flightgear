/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program realizes the usage of the VoIP infractructure based
 * on flight data which is send from FlightGear with an external
 * protocol to this application.
 *
 * Clement de l'Hamaide - Jan 2014
 * Re-writting of FGCom standalone
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#include "config.h"

#include <simgear/sg_inlines.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

#include <iaxclient.h>

#include "fgcom_external.hxx"
#include "positions.hxx" // provides _positionsData[];

int         _port           = 16661;
int         _callId         = -1;
int         _currentFreqKhz = -1;
int         _maxRange       = 100;
int         _minRange       = 10;
int         _registrationId = -1;
bool        _libInitialized = false;
bool        _running        = true;
bool        _debug          = false;
bool        _connected      = false;
double      _frequency      = -1;
double      _atis           = -1;
double      _silenceThd     = -35.0;
std::string _app            = "FGCOM-";
std::string _host           = "127.0.0.1";
std::string _server         = "fgcom.flightgear.org";
std::string _airport        = "ZZZZ";
std::string _callsign       = "guest";
std::string _username       = "guest";
std::string _password       = "guest";

SGGeod      _airportPos;
SGTimeStamp _p;
std::multimap<int, Airport> _airportsData;

const int special_freq[] = { // Define some freq who need to be used with icao = ZZZZ
	910000,
	911000,
	700000,
	123450,
	122750,
	121500,
	123500 };

//
// Main loop
//

int main(int argc, char** argv)
{
    signal(SIGINT,  quit);
    signal(SIGTERM, quit);

    simgear::requestConsole(true);
    sglog().setLogLevels(SG_ALL, SG_INFO);
    _app += FGCOM_VERSION;
    Modes mode          = PILOT;
    std::string num     = "";

    for(int count = 1; count < argc; count++) {
        std::string item = argv[count];
        std::string option = item.substr(2, item.find("=")-2);
        std::string value = item.substr(item.find("=")+1, item.size());
        if(option == "server")             _server        = value;
        if(option == "host")               _host          = value;
        if(option == "port")               _port          = atoi(value.c_str());
        if(option == "callsign")           _callsign      = value;
        if(option == "frequency")          _frequency     = atof(value.c_str());
        if(option == "atis")               _atis          = atof(value.c_str());
        if(option == "airport")            _airport       = simgear::strutils::uppercase(value);
        if(option == "username")           _username      = value;
        if(option == "password")           _password      = value;
        if(option == "silence-threshold")  _silenceThd    = atof(value.c_str());
        if(option == "debug")               sglog().setLogLevels(SG_ALL, SG_DEBUG);
        if(option == "help")                return usage();
        if(option == "version")             return version();
    }

    if(_frequency == 910.000)
        mode = TEST;
    if(_frequency <= 136.975 && _frequency >= 118.000)
        mode = OBS;
    if(_atis <= 136.975 && _atis >= 118.000 && _airport != "ZZZZ")
        mode = ATC;

    SG_LOG(SG_GENERAL, SG_INFO, "FGCom " << FGCOM_VERSION << " compiled " << __DATE__
                                << ", at " << __TIME__ );
    SG_LOG(SG_GENERAL, SG_INFO, "For help usage, use --help");
    SG_LOG(SG_GENERAL, SG_INFO, "Starting FGCom session as " << _username << ":xxxxxxxxx@" << _server);
    
    if( !(_libInitialized = lib_init()) )
        return EXIT_FAILURE;

    if (_username != "guest" && _password != "guest")
        _registrationId = lib_registration();
  
    if(mode == PILOT) {
        SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode PILOT" );

        simgear::Socket::initSockets();
        simgear::Socket sgSocket;
        sgSocket.open(false);
        sgSocket.bind(_host.c_str(), _port);
        sgSocket.setBlocking(false);
        lib_setVolume(0.0, 1.0);
        static char currentPacket[MAXBUFLEN+2], previousPacket[MAXBUFLEN+2];
        struct Data currentData, previousData, previousPosData;
        double currentFreq = -1, previousFreq = -1;
        std::string currentIcao = "";
        ActiveComm activeComm = COM1;

        _airportsData = getAirportsData();
        SG_LOG(SG_GENERAL, SG_INFO, "");

        while(_running) {
            int bytes = sgSocket.recv(currentPacket, sizeof(currentPacket)-1, 0);
            if (bytes == -1) {
                SGTimeStamp::sleepForMSec(1);  // Prevent full CPU usage (loop)
                continue;
            }

            currentPacket[bytes] = '\0';
            if( strcmp(currentPacket, previousPacket) != 0 ) {
                std::string packet(currentPacket);
                std::vector<std::string> properties = simgear::strutils::split(packet, ",");
                for(size_t i=0; i < properties.size(); i++) {
                    std::vector<std::string> prop = simgear::strutils::split(properties[i], "=");
                    if(prop[0] == "PTT")         currentData.ptt         = atoi(prop[1].c_str());
                    if(prop[0] == "LAT")         currentData.lat         = atof(prop[1].c_str());
                    if(prop[0] == "LON")         currentData.lon         = atof(prop[1].c_str());
                    if(prop[0] == "ALT")         currentData.alt         = atof(prop[1].c_str());
                    if(prop[0] == "COM1_FRQ")    currentData.com1        = atof(prop[1].c_str());
                    if(prop[0] == "COM2_FRQ")    currentData.com2        = atof(prop[1].c_str());
                    if(prop[0] == "OUTPUT_VOL")  currentData.outputVol   = atof(prop[1].c_str());
                    if(prop[0] == "SILENCE_THD") currentData.silenceThd  = atof(prop[1].c_str());
                    if(prop[0] == "CALLSIGN")    currentData.callsign    = prop[1];
                }

                if(currentData.ptt != previousData.ptt) {
                    if(currentData.ptt == 2) {
                        if(activeComm == COM1) {
                            activeComm = COM2;
                            currentFreq = currentData.com2;
                        } else {
                            activeComm = COM1;
                            currentFreq = currentData.com1;
                        }
                        SG_LOG( SG_GENERAL, SG_INFO, "Select radio " << activeComm << " on " << currentFreq << " MHz" );
                    } else if(currentData.ptt) {
                        SG_LOG( SG_GENERAL, SG_INFO, "[SPEAK] unmute mic, mute speaker" );
                        lib_setVolume(1.0, 0.0);
                    } else {
                        SG_LOG( SG_GENERAL, SG_INFO, "[LISTEN] mute mic, unmute speaker" );
                        lib_setVolume(0.0, currentData.outputVol);
                    }
                }

                if(currentData.outputVol != previousData.outputVol)
                    lib_setVolume(0.0, currentData.outputVol);

                if(currentData.silenceThd != previousData.silenceThd)
                    lib_setSilenceThreshold(currentData.silenceThd);

                if(currentData.callsign != previousData.callsign)
                    lib_setCallerId(currentData.callsign);

                if(currentData.com1 != previousData.com1 && activeComm == COM1) {
                    currentFreq = currentData.com1;
                    SG_LOG( SG_GENERAL, SG_INFO, "Select frequency " << currentFreq << " MHz on radio " << activeComm );
                }

                if(currentData.com2 != previousData.com2 && activeComm == COM2) {
                    currentFreq = currentData.com2;
                    SG_LOG( SG_GENERAL, SG_INFO, "Select frequency " << currentFreq << " MHz on radio " << activeComm );
                }

                if(previousFreq != currentFreq || currentData.callsign != previousData.callsign) {
                    _currentFreqKhz = 10 * static_cast<int>(currentFreq * 100 + 0.25);
                    currentIcao = getClosestAirportForFreq(currentFreq, currentData.lat, currentData.lon, currentData.alt);

                    if(isInRange(currentIcao, currentData.lat, currentData.lon, currentData.alt)) {
                        _connected = lib_call(currentIcao, currentFreq);
                        SG_LOG( SG_GENERAL, SG_INFO, "Connecting " << currentIcao << " on " << currentFreq << " MHz" );
                    } else {
                        if(_connected) {
                            _connected = lib_hangup();
                            SG_LOG( SG_GENERAL, SG_INFO, "Disconnecting " << currentIcao << " on " << currentFreq << " MHz (out of range)" );
                        }
                    }
                }

                if( currentData.lat <= previousPosData.lat - 0.05  ||
                    currentData.lon <= previousPosData.lon - 0.05  ||
                    currentData.alt <= previousPosData.alt - 50.0  ||
                    currentData.lat >= previousPosData.lat + 0.05  ||
                    currentData.lon >= previousPosData.lon + 0.05  ||
                    currentData.alt >= previousPosData.alt + 50.0) {

                    currentIcao = getClosestAirportForFreq(currentFreq, currentData.lat, currentData.lon, currentData.alt);
                    if(_connected) {
                        if(!isInRange(currentIcao, currentData.lat, currentData.lon, currentData.alt)) {
                            _connected = lib_hangup();
                            SG_LOG( SG_GENERAL, SG_INFO, "Disconnecting " << currentIcao << " on " << currentFreq << " MHz (out of range)" );
                        }
                    } else {
                        if(isInRange(currentIcao, currentData.lat, currentData.lon, currentData.alt)) {
                            _connected = lib_call(currentIcao, currentFreq);
                            SG_LOG( SG_GENERAL, SG_INFO, "Connecting " << currentIcao << " on " << currentFreq << " MHz" );
                        }
                    }
                    previousPosData = currentData;
                }
                previousFreq = currentFreq;
                previousData = currentData;
            }
            strcpy(previousPacket, currentPacket);
        } // while()
        sgSocket.close();
    } else { // if(mode == PILOT)
        int sessionDuration = 1000;
        _p.stamp();
        if(mode == OBS) {
            SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode OBS (max duration: 6 hours)" );
            sessionDuration *= 2160; // 6 hours for OBS mode
            lib_setVolume(0.0, 1.0);
            lib_setCallerId("::OBS::");
            num = computePhoneNumber(_frequency, _airport);
        } else {
            lib_setVolume(1.0, 1.0);
            if(mode == TEST) {
                sessionDuration *= 65; // 65 seconds for TEST mode
                SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode TEST (max duration: 65 seconds)" );
                _airport = "ZZZZ";
                num = computePhoneNumber(_frequency, _airport);
            } else if(mode == ATC) {
                sessionDuration *= 45; // 45 seconds for ATC mode
                SG_LOG( SG_GENERAL, SG_DEBUG, "Entering main loop in mode ATC (max duration: 45 seconds)" );
                num = computePhoneNumber(_atis, _airport, true);
            }
        }
        _connected = lib_directCall(_airport, _frequency, num);

        while (_p.elapsedMSec() <= sessionDuration && _running){
            SGTimeStamp::sleepForMSec(2000);
        }
    }

    if(!lib_shutdown())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

// function: getAirportsData
// action: parse positionsData.hxx then build multimap

std::multimap<int, Airport> getAirportsData()
{
    std::vector<std::string> lines;
    std::multimap<int, Airport> aptData;
    SG_LOG(SG_GENERAL, SG_INFO, "Loading airports information...");

    for(size_t i=0; i < sizeof(_positionsData)/sizeof(*_positionsData); i++) { // _positionsData is provided by positions.hxx
        std::vector<std::string> entries = simgear::strutils::split(_positionsData[i], ",");
        if(entries.size() == 6) {
            // [0]=ICAO, [1]=Frequency, [2]=Latitude, [3]=Longitude, [4]=ID/Type, [5]=Name
            std::string entryIcao  = entries[0];
            double      entryFreq  = atof(entries[1].c_str());
            double      entryLat   = atof(entries[2].c_str());
            double      entryLon   = atof(entries[3].c_str());
            std::string entryType  = entries[4];
            std::string entryName  = entries[5];

            int aptFreqKhz = 10 * static_cast<int>(entryFreq * 100 + 0.25);
            Airport apt;
                    apt.icao       = entryIcao;
                    apt.frequency  = entryFreq;
                    apt.latitude   = entryLat;
                    apt.longitude  = entryLon;
                    apt.type       = entryType;
                    apt.name       = entryName;
            aptData.insert( std::pair<int, Airport>(aptFreqKhz, apt) );
        }
    }
    return aptData;
}

// function: orderByDistanceNm
// action: sort airportsInRange vector by distanceNm ASC in getClosestAirportForFreq()

bool orderByDistanceNm(Airport a, Airport b)
{
    return a.distanceNm < b.distanceNm;
}

// function: gestClosestAircraftForFreq
// action: return ICAO of closest airport with given frequency and define his position

std::string getClosestAirportForFreq(double freq, double acftLat, double acftLon, double acftAlt)
{
    for(size_t i=0; i<sizeof(special_freq)/sizeof(special_freq[0]); i++) { // Check if it's a special freq
      if( special_freq[i] == _currentFreqKhz )
        return std::string("ZZZZ");
    }

    std::string icao = "";
    double aptLon    = 0;
    double aptLat    = 0;
    int freqKhz      = 10 * static_cast<int>(freq * 100 + 0.25);
    SGGeod acftPos   = SGGeod::fromDegFt(acftLon, acftLat, acftAlt);
    std::vector<Airport> airportsInRange;

    std::pair <std::multimap<int, Airport>::iterator, std::multimap<int, Airport>::iterator> ret;
    ret = _airportsData.equal_range(freqKhz);
    for (std::multimap<int, Airport>::iterator it=ret.first; it!=ret.second; ++it) {
        SGGeod aptPos = SGGeod::fromDeg(it->second.longitude, it->second.latitude);
        double distNm = SGGeodesy::distanceNm(aptPos, acftPos);
        if(distNm <= _maxRange){
            it->second.distanceNm = distNm;
            airportsInRange.push_back(it->second);
        }
    }

    if(!airportsInRange.size())
        return icao;

    std::sort(airportsInRange.begin(), airportsInRange.end(), orderByDistanceNm);

    aptLon      = airportsInRange[0].longitude;
    aptLat      = airportsInRange[0].latitude;
    icao        = airportsInRange[0].icao;
    _airportPos = SGGeod::fromDeg(aptLon, aptLat);

    SG_LOG(SG_GENERAL, SG_INFO, "Airport " << airportsInRange[0].icao << " " << airportsInRange[0].name << " - "
                                << airportsInRange[0].type << " on " << airportsInRange[0].frequency
                                << " - is in range " << airportsInRange[0].distanceNm << "nm ("
                                << (SG_NM_TO_METER*airportsInRange[0].distanceNm)/1000 <<"km)");
    return icao;
}

// function: isInRange
// action: return TRUE if airport/freq is in range, else return FALSE

bool isInRange(std::string icao, double acftLat, double acftLon, double acftAlt)
{
    for(size_t i=0; i<sizeof(special_freq)/sizeof(special_freq[0]); i++) { // Check if it's a special freq
      if( special_freq[i] == _currentFreqKhz )
        return true;
    }

    if(icao.empty())
        return false;

    SGGeod acftPos = SGGeod::fromDegFt(acftLon, acftLat, acftAlt);
    double distNm = SGGeodesy::distanceNm(_airportPos, acftPos);
    double delta_elevation_ft = fabs(acftPos.getElevationFt() - _airportPos.getElevationFt());
    double rangeNm = 1.23 * sqrt(delta_elevation_ft);

    if (rangeNm > _maxRange) rangeNm = _maxRange;
    if (rangeNm < _minRange) rangeNm = _minRange;
    if( distNm > rangeNm )   return false;
    return true;
}

// function: quit
// action: set _running flag to false

void quit(int state)
{
    SG_LOG( SG_GENERAL, SG_INFO, "Exiting FGCom" );
    _running = false;
#ifdef _WIN32
    lib_shutdown();
    SG_LOG(SG_GENERAL, SG_INFO, "You can close the terminal now");
#endif
}

// function: usage
// action: display FGCom usage then quit

int usage()
{
    std::cout << "FGCom " << FGCOM_VERSION << " usage:" << std::endl;
    std::cout << "        --server=fgcom.flightgear.org   -  Server to connect" << std::endl;
    std::cout << "        --host=127.0.0.1                -  Host to listen i.e where FG is running" << std::endl;
    std::cout << "        --port=16661                    -  Port to use" << std::endl;
    std::cout << "        --callsign=guest                -  Callsign during session e.g F-ELYD" << std::endl;
    std::cout << "        --frequency=xxx.xxx             -  Frequency e.g 120.500" << std::endl;
    std::cout << "        --airport=YYYY                  -  ICAO of airport e.g KSFO" << std::endl;
    std::cout << "        --username=guest                -  Username for registration" << std::endl;
    std::cout << "        --password=guest                -  Password for registration" << std::endl;
    std::cout << "        --silence-threshold=-35         -  Silence threshold in dB (-60 < range < 0 )" << std::endl;
    std::cout << "        --debug                         -  Enable debug output" << std::endl;
    std::cout << "        --help                          -  Show this message" << std::endl;
    std::cout << "        --version                       -  Show version" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  None of these options are required, you can simply start FGCom without option at all: it works" << std::endl;
    std::cout << "  For further information, please visit: http://wiki.flightgear.org/FGCom_3.0" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  About silence-threshold:" << std::endl;
    std::cout << "    This is the limit, in dB, when FGCom consider no voice in your microphone." << std::endl;
    std::cout << "    --silence-threshold=-60 is similar to micro always ON" << std::endl;
    std::cout << "    --silence-threshold=0 is similar to micro always OFF" << std::endl;
    std::cout << "    Default value is -35.0 dB" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  In order to make an echo-test, you have to start FGCom like:" << std::endl;
    std::cout << "    fgcom --frequency=910" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  In order to listen a frequency, you have to start FGCom like:" << std::endl;
    std::cout << "    fgcom --frequency=xxx.xxx --airport=YYYY" << std::endl;
    std::cout << "    where xxx.xxx is the frequency of the ICAO airport YYYY that you want to listen to" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  In order to record an ATIS message, you have to start FGCom like:" << std::endl;
    std::cout << "    fgcom --atis=xxx.xxx --airport=YYYY" << std::endl;
    std::cout << "    where xxx.xxx is the ATIS frequency of the ICAO airport YYYY" << std::endl;
    std::cout << "" << std::endl;
    return EXIT_SUCCESS;
}

// function: version
// action: display FGCom version then quit

int version()
{
    SG_LOG(SG_GENERAL, SG_INFO, "FGCom " << FGCOM_VERSION << " compiled " << __DATE__
                                << ", at " << __TIME__ );
    std::cout << "" << std::endl;
    return EXIT_SUCCESS;
}

// function: computePhoneNumber
// action: return phone number

std::string computePhoneNumber(double freq, std::string icao, bool atis)
{
    if(icao.empty())
        return std::string(); 

    char phoneNumber[256];
    char exten[32];
    char tmp[5];
    int  prefix = atis ? 99 : 01;

    sprintf( tmp, "%4s", icao.c_str() );

    sprintf( exten,
             "%02d%02d%02d%02d%02d%06d",
             prefix,
             tmp[0],
             tmp[1],
             tmp[2],
             tmp[3],
             (int) (freq * 1000 + 0.5) );
    exten[16] = '\0';

    snprintf( phoneNumber,
              sizeof(phoneNumber),
              "%s:%s@%s/%s",
              _username.c_str(),
              _password.c_str(),
              _server.c_str(),
              exten);
    return phoneNumber;
}

// function: lib_setVolume
// action: set input/output volume

void lib_setVolume(double input, double output)
{
    SG_CLAMP_RANGE<double>(input, 0.0, 1.0);
    SG_CLAMP_RANGE<double>(output, 0.0, 1.0);
    SG_LOG(SG_GENERAL, SG_DEBUG, "Set volume input=" << input << " , output=" << output);
    iaxc_input_level_set(input);
    iaxc_output_level_set(output);
}

// function: lib_setSilenceThreshold
// action: set silence threshold

void lib_setSilenceThreshold(double thd)
{
    SG_CLAMP_RANGE<double>(thd, -60, 0);
    SG_LOG(SG_GENERAL, SG_DEBUG, "Set silence threshold=" << thd);
    iaxc_set_silence_threshold(thd);
}

// function: lib_setCallerId
// action: set caller id for the session

void lib_setCallerId(std::string callsign)
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Set caller ID=" << callsign);
    iaxc_set_callerid (callsign.c_str(), _app.c_str());
}

// function: lib_init
// action: init the library

bool lib_init()
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Initializing IAX library");
#ifdef _MSC_VER
    iaxc_set_networking( (iaxc_sendto_t)sendto, (iaxc_recvfrom_t)recvfrom );
#endif
    if (iaxc_initialize(4)) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Error: cannot initialize IAXClient !\nHINT: Have you checked the mic and speakers ?" );
        return false;
    }

    iaxc_set_callerid( _callsign.c_str(), _app.c_str() );
    iaxc_set_formats(IAXC_FORMAT_SPEEX, IAXC_FORMAT_ULAW|IAXC_FORMAT_SPEEX);
    iaxc_set_speex_settings(1, 5, 0, 1, 0, 3);
    iaxc_set_filters(IAXC_FILTER_AGC | IAXC_FILTER_DENOISE);
    iaxc_set_event_callback(iaxc_callback);
    iaxc_start_processing_thread ();
    lib_setSilenceThreshold(_silenceThd);
    return true;
}

// function: lib_shutdown
// action: stop the library

bool lib_shutdown()
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Shutdown IAX library");
    lib_hangup();
    if(_registrationId != -1)
        iaxc_unregister(_registrationId);
    return true;
}

// function: lib_call
// action: register a user on remote server then return the registration ID

int lib_registration()
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Request registration");
    SG_LOG(SG_GENERAL, SG_DEBUG, "           username: " << _username);
    SG_LOG(SG_GENERAL, SG_DEBUG, "           password: xxxxxxxx");
    SG_LOG(SG_GENERAL, SG_DEBUG, "           server:   " << _server);
    int regId = iaxc_register( _username.c_str(), _password.c_str(), _server.c_str());
    if(regId == -1) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Warning: cannot register '" << _username << "' at '" << _server );
    }
    return regId;
}

// function: lib_call
// action: kill current call then do a new call

bool lib_call(std::string icao, double freq)
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Request new call");
    SG_LOG(SG_GENERAL, SG_DEBUG, "           icao: " << icao);
    SG_LOG(SG_GENERAL, SG_DEBUG, "           freq: " << freq);
    lib_hangup();
    iaxc_millisleep(300);
    std::string num = computePhoneNumber(freq, icao);
    if(num.empty())
        return false;
    _callId = iaxc_call(num.c_str());
    if(_callId == -1) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Warning: cannot call: " << num );
        return false;
    }
    return true;
}

bool lib_directCall(std::string icao, double freq, std::string num)
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Request new call");
    SG_LOG(SG_GENERAL, SG_DEBUG, "           icao: " << icao);
    SG_LOG(SG_GENERAL, SG_DEBUG, "           freq: " << freq);
    lib_hangup();
    iaxc_millisleep(300);
    if(num.empty())
        return false;
    _callId = iaxc_call(num.c_str());
    if(_callId == -1) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Warning: cannot call: " << num );
        return false;
    }
    return true;
}

// function: lib_hangup
// action: kill current call

bool lib_hangup()
{
    if(!_connected)
        return false;
    SG_LOG(SG_GENERAL, SG_DEBUG, "Request hangup");
    iaxc_dump_all_calls();
    _callId = -1;
    return false;
}

// function: iaxc_callback
// action: parse IAX event then call event handler

int iaxc_callback(iaxc_event e)
{
    switch (e.type) {
        case IAXC_EVENT_TEXT:
            if(e.ev.text.type == IAXC_TEXT_TYPE_STATUS ||
               e.ev.text.type == IAXC_TEXT_TYPE_IAX)
                   SG_LOG( SG_GENERAL, SG_INFO, "Message: " << e.ev.text.message );
            break;
    }
    return 1;
}
// eof
