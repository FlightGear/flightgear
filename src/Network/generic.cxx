// generic.cxx -- generic protocol class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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

#include <string.h>                // strstr()
#include <stdlib.h>                // strtod(), atoi()
#include <cstdio>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_os.hxx>
#include <Main/util.hxx>
#include "generic.hxx"

using simgear::strutils::unescape;

class FGProtocolWrapper {
public:
  virtual ~FGProtocolWrapper() {}
  virtual int wrap( size_t n, uint8_t * buf ) = 0;
  virtual int unwrap( size_t n, uint8_t * buf ) = 0;
};

/**
 * http://www.ka9q.net/papers/kiss.html
 */
class FGKissWrapper : public FGProtocolWrapper {
public:
  virtual int wrap( size_t n, uint8_t * buf );
  virtual int unwrap( size_t n, uint8_t * buf );
private:
  static const uint8_t  FEND;
  static const uint8_t  FESC;
  static const uint8_t  TFEND;
  static const uint8_t  TFESC;
};

const uint8_t  FGKissWrapper::FEND  = 0xC0;
const uint8_t  FGKissWrapper::FESC  = 0xDB;
const uint8_t  FGKissWrapper::TFEND = 0xDC;
const uint8_t  FGKissWrapper::TFESC = 0xDD;

int FGKissWrapper::wrap( size_t n, uint8_t * buf )
{
  std::vector<uint8_t> dest;
  uint8_t *sp = buf;

  dest.push_back(FEND);
  dest.push_back(0); // command/channel always zero
  for( size_t i = 0; i < n; i++ ) {
    uint8_t c = *sp++;
    switch( c ) {
      case FESC:
        dest.push_back(FESC);
        dest.push_back(TFESC);
        break;

      case FEND:
        dest.push_back(FESC);
        dest.push_back(TFEND);
        break;

      default:
        dest.push_back(c);
        break;
    }
  }
  dest.push_back(FEND);

  memcpy( buf, dest.data(), dest.size() );
  return dest.size();
}

int FGKissWrapper::unwrap( size_t n, uint8_t * buf )
{
  uint8_t * sp = buf;

  // look for FEND
  while( 0 < n && FEND != *sp ) {
    sp++;
    n--;
  }

  // ignore all leading FEND
  while( 0 < n && FEND == *sp ) {
    sp++;
    n--;
  }

  if( 0 == n ) return 0;

  std::vector<uint8_t> dest;
  {
    bool escaped = false;

    while( 0 < n ) {

      n--;
      uint8_t c = *sp++;

      if( escaped ) {
        switch( c ) {

          case TFESC:
            dest.push_back( FESC );
            break;

          case TFEND:
            dest.push_back( FEND );
            break;

          default: // this is an error - ignore and continue
            break;
        }

        escaped = false;

      } else {

        switch( c ) {
          case FESC:
            escaped = true;
            break;

          case FEND:
            if( 0 != n ) {
              SG_LOG(SG_IO, SG_WARN,
               "KISS frame detected FEND before end of frame. Trailing data dropped." );
            }
            n = 0; 
            break;

          default:
            dest.push_back( c );
            break;
        }
      }
    }
  }

  memcpy( buf, dest.data(), dest.size() );
  return dest.size();
}


class FGSTXETXWrapper : public FGProtocolWrapper {
public:
  virtual int wrap( size_t n, uint8_t * buf );
  virtual int unwrap( size_t n, uint8_t * buf );

  static const uint8_t  STX;
  static const uint8_t  ETX;
  static const uint8_t  DLE;
};

const uint8_t  FGSTXETXWrapper::STX  = 0x02;
const uint8_t  FGSTXETXWrapper::ETX  = 0x03;
const uint8_t  FGSTXETXWrapper::DLE  = 0x00;

int FGSTXETXWrapper::wrap( size_t n, uint8_t * buf )
{
  // stuff payload as
  // <dle><stx>payload<dle><etx>
  // if payload contains <dle>, stuff <dle> as <dle><dle>
  std::vector<uint8_t> dest;
  uint8_t *sp = buf;

  dest.push_back(DLE);
  dest.push_back(STX);

  while( n > 0 ) {
    n--;

    if( DLE == *sp )
      dest.push_back(DLE);

    dest.push_back(*sp++);
  }

  dest.push_back(DLE);
  dest.push_back(ETX);

  memcpy( buf, dest.data(), dest.size() ); 
  return dest.size();
}

int FGSTXETXWrapper::unwrap( size_t n, uint8_t * buf )
{
  return n;
}

FGGeneric::FGGeneric(vector<string> tokens) : exitOnError(false), initOk(false), wrapper(NULL)
{
    size_t configToken;
    if (tokens[1] == "socket") {
        configToken = 7;
    } else if (tokens[1] == "file") {
        configToken = 5;
    } else {
        configToken = 6; 
    }

    if ((configToken >= tokens.size())||(tokens[ configToken ] == "")) {
       SG_LOG(SG_NETWORK, SG_ALERT,
              "Not enough tokens passed for generic '" << tokens[1] << "' protocol. ");
       return;
    }

    string config = tokens[ configToken ];
    file_name = config+".xml";
    direction = tokens[2];

    if (direction != "in" && direction != "out" && direction != "bi") {
        SG_LOG(SG_NETWORK, SG_ALERT, "Unsuported protocol direction: "
               << direction);
        return;
    }

    reinit();
}

FGGeneric::~FGGeneric() {
  delete wrapper;
}

union u32 {
    uint32_t intVal;
    float floatVal;
};

union u64 {
    uint64_t longVal;
    double doubleVal;
};

// generate the message
bool FGGeneric::gen_message_binary() {
    string generic_sentence;
    length = 0;

    double val;
    for (unsigned int i = 0; i < _out_message.size(); i++) {

        switch (_out_message[i].type) {
        case FG_INT:
        {
            val = _out_message[i].offset +
                  _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            int32_t intVal = val;
            if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
                intVal = (int32_t) sg_bswap_32((uint32_t)intVal);
            }
            memcpy(&buf[length], &intVal, sizeof(int32_t));
            length += sizeof(int32_t);
            break;
        }

        case FG_BOOL:
            buf[length] = (char) (_out_message[i].prop->getBoolValue() ? true : false);
            length += 1;
            break;

        case FG_FIXED:
        {
            val = _out_message[i].offset +
                 _out_message[i].prop->getFloatValue() * _out_message[i].factor;

            int32_t fixed = (int)(val * 65536.0f);
            if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
                fixed = (int32_t) sg_bswap_32((uint32_t)fixed);
            } 
            memcpy(&buf[length], &fixed, sizeof(int32_t));
            length += sizeof(int32_t);
            break;
        }

        case FG_FLOAT:
        {
            val = _out_message[i].offset +
                 _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            u32 tmpun32;
            tmpun32.floatVal = static_cast<float>(val);

            if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
                tmpun32.intVal = sg_bswap_32(tmpun32.intVal);
            }
            memcpy(&buf[length], &tmpun32.intVal, sizeof(uint32_t));
            length += sizeof(uint32_t);
            break;
        }

        case FG_DOUBLE:
        {
            val = _out_message[i].offset +
                 _out_message[i].prop->getDoubleValue() * _out_message[i].factor;
            u64 tmpun64;
            tmpun64.doubleVal = val;

            if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
                tmpun64.longVal = sg_bswap_64(tmpun64.longVal);
            }
            memcpy(&buf[length], &tmpun64.longVal, sizeof(uint64_t));
            length += sizeof(uint64_t);
            break;
        }

        case FG_BYTE:
        {
            val = _out_message[i].offset +
                  _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            int8_t byteVal = val;
            memcpy(&buf[length], &byteVal, sizeof(int8_t));
            length += sizeof(int8_t);
            break;
        }

        case FG_WORD:
        {
            val = _out_message[i].offset +
                  _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            int16_t wordVal = val;
            memcpy(&buf[length], &wordVal, sizeof(int16_t));
            length += sizeof(int16_t);
            break;
        }

        default: // SG_STRING
            const char *strdata = _out_message[i].prop->getStringValue();
            int32_t strlength = strlen(strdata);

            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                SG_LOG( SG_IO, SG_ALERT, "Generic protocol: "
                        "FG_STRING will be written in host byte order.");
            }
            /* Format for strings is 
             * [length as int, 4 bytes][ASCII data, length bytes]
             */
            if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
                strlength = sg_bswap_32(strlength);
            }
            memcpy(&buf[length], &strlength, sizeof(int32_t));
            length += sizeof(int32_t);
            strncpy(&buf[length], strdata, strlength);
            length += strlength; 
            /* FIXME padding for alignment? Something like: 
             * length += (strlength % 4 > 0 ? sizeof(int32_t) - strlength % 4 : 0;
             */
            break;

        }
    }

    // add the footer to the packet ("line")
    switch (binary_footer_type) {
        case FOOTER_LENGTH:
            binary_footer_value = length;
            break;

        case FOOTER_MAGIC:
        case FOOTER_NONE:
            break;
    }

    if (binary_footer_type != FOOTER_NONE) {
        int32_t intValue = binary_footer_value;
        if (binary_byte_order != BYTE_ORDER_MATCHES_NETWORK_ORDER) {
            intValue = sg_bswap_32(binary_footer_value);
        }
        memcpy(&buf[length], &intValue, sizeof(int32_t));
        length += sizeof(int32_t);
    }

    if( wrapper ) length = wrapper->wrap( length, reinterpret_cast<uint8_t*>(buf) );

    return true;
}

bool FGGeneric::gen_message_ascii() {
    string generic_sentence;
    char tmp[255];
    length = 0;

    double val;
    for (unsigned int i = 0; i < _out_message.size(); i++) {

        if (i > 0) {
            generic_sentence += var_separator;
        }
        
        string format = simgear::strutils::sanitizePrintfFormat(_out_message[i].format);

        switch (_out_message[i].type) {
        case FG_BYTE:
        case FG_WORD:
        case FG_INT:
            val = _out_message[i].offset +
                  _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            snprintf(tmp, 255, format.c_str(), (int)val);
            break;

        case FG_BOOL:
            snprintf(tmp, 255, format.c_str(),
                               _out_message[i].prop->getBoolValue());
            break;

        case FG_FIXED:
            val = _out_message[i].offset +
                _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            snprintf(tmp, 255, format.c_str(), (float)val);
            break;

        case FG_FLOAT:
            val = _out_message[i].offset +
                _out_message[i].prop->getFloatValue() * _out_message[i].factor;
            snprintf(tmp, 255, format.c_str(), (float)val);
            break;

        case FG_DOUBLE:
            val = _out_message[i].offset +
                _out_message[i].prop->getDoubleValue() * _out_message[i].factor;
            snprintf(tmp, 255, format.c_str(), (double)val);
            break;

        default: // SG_STRING
            snprintf(tmp, 255, format.c_str(),
                                   _out_message[i].prop->getStringValue());
        }

        generic_sentence += tmp;
    }

    /* After each lot of variables has been added, put the line separator
     * char/string
     */
    generic_sentence += line_separator;

    length =  generic_sentence.length();
    strncpy( buf, generic_sentence.c_str(), length );

    return true;
}

bool FGGeneric::gen_message() {
    if (binary_mode) {
        return gen_message_binary();
    } else {
        return gen_message_ascii();
    }
}

bool FGGeneric::parse_message_binary(int length) {
    char *p2, *p1 = buf;
    int32_t tmp32;
    int i = -1;

    p2 = p1 + length;
    while ((++i < (int)_in_message.size()) && (p1  < p2)) {

        switch (_in_message[i].type) {
        case FG_INT:
            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                tmp32 = sg_bswap_32(*(int32_t *)p1);
            } else {
                tmp32 = *(int32_t *)p1;
            }
            updateValue(_in_message[i], (int)tmp32);
            p1 += sizeof(int32_t);
            break;

        case FG_BOOL:
            updateValue(_in_message[i], p1[0] != 0);
            p1 += 1;
            break;

        case FG_FIXED:
            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                tmp32 = sg_bswap_32(*(int32_t *)p1);
            } else {
                tmp32 = *(int32_t *)p1;
            }
            updateValue(_in_message[i], (float)tmp32 / 65536.0f);
            p1 += sizeof(int32_t);
            break;

        case FG_FLOAT:
            u32 tmpun32;
            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                tmpun32.intVal = sg_bswap_32(*(uint32_t *)p1);
            } else {
                tmpun32.floatVal = *(float *)p1;
            }
            updateValue(_in_message[i], tmpun32.floatVal);
            p1 += sizeof(int32_t);
            break;

        case FG_DOUBLE:
            u64 tmpun64;
            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                tmpun64.longVal = sg_bswap_64(*(uint64_t *)p1);
            } else {
                tmpun64.doubleVal = *(double *)p1;
            }
            updateValue(_in_message[i], tmpun64.doubleVal);
            p1 += sizeof(int64_t);
            break;

        case FG_BYTE:
            tmp32 = *(int8_t *)p1;
            updateValue(_in_message[i], (int)tmp32);
            p1 += sizeof(int8_t);
            break;

        case FG_WORD:
            if (binary_byte_order == BYTE_ORDER_NEEDS_CONVERSION) {
                tmp32 = sg_bswap_16(*(int16_t *)p1);
            } else {
                tmp32 = *(int16_t *)p1;
            }
            updateValue(_in_message[i], (int)tmp32);
            p1 += sizeof(int16_t);
            break;

        default: // SG_STRING
            SG_LOG( SG_IO, SG_ALERT, "Generic protocol: "
                    "Ignoring unsupported binary input chunk type.");
            break;
        }
    }
    
    return true;
}

bool FGGeneric::parse_message_ascii(int length) {
    char *p1 = buf;
    int i = -1;
    int chunks = _in_message.size();
    int line_separator_size = line_separator.size();

    if (length < line_separator_size ||
        line_separator.compare(buf + length - line_separator_size) != 0) {

        SG_LOG(SG_IO, SG_WARN,
               "Input line does not end with expected line separator." );
    } else {
        buf[length - line_separator_size] = 0;
    }

    size_t varsep_len = var_separator.length();
    while ((++i < chunks) && p1) {
        char* p2 = NULL;

        if (varsep_len > 0)
        {
            p2 = strstr(p1, var_separator.c_str());
            if (p2) {
                *p2 = 0;
                p2 += varsep_len;
            }
        }

        switch (_in_message[i].type) {
        case FG_BYTE:
        case FG_WORD:
        case FG_INT:
            updateValue(_in_message[i], atoi(p1));
            break;

        case FG_BOOL:
            updateValue(_in_message[i], atof(p1) != 0.0);
            break;

        case FG_FIXED:
        case FG_FLOAT:
            updateValue(_in_message[i], (float)strtod(p1, 0));
            break;

        case FG_DOUBLE:
            updateValue(_in_message[i], (double)strtod(p1, 0));
            break;

        default: // SG_STRING
            _in_message[i].prop->setStringValue(p1);
            break;
        }

        p1 = p2;
    }

    return true;
}

bool FGGeneric::parse_message_len(int length) {
    if (binary_mode) {
        return parse_message_binary(length);
    } else {
        return parse_message_ascii(length);
    }
}


// open hailing frequencies
bool FGGeneric::open() {
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
                << "is already in use, ignoring" );
        return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
        return false;
    }

    set_enabled( true );

    if ( ((get_direction() == SG_IO_OUT )||
          (get_direction() == SG_IO_BI))
          && ! preamble.empty() ) {
        if ( ! io->write( preamble.c_str(), preamble.size() ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing preamble." );
            return false;
        }
    }

    return true;
}


// process work for this port
bool FGGeneric::process() {
    SGIOChannel *io = get_io_channel();

    if ( (get_direction() == SG_IO_OUT) ||
         (get_direction() == SG_IO_BI) ) {
        gen_message();
        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_WARN, "Error writing data." );
            goto error_out;
        }
    }

    if (( get_direction() == SG_IO_IN ) ||
        (get_direction() == SG_IO_BI) ) {
        if ( io->get_type() == sgFileType ) {
            if (!binary_mode) {
                length = io->readline( buf, FG_MAX_MSG_SIZE );
                if ( length > 0 ) {
                    parse_message_len( length );
                } else {
                    SG_LOG( SG_IO, SG_ALERT, "Error reading data." );
                    return false;
                }
            } else {
                length = io->read( buf, binary_record_length );
                if ( length == binary_record_length ) {
                    parse_message_len( length );
                } else {
                    SG_LOG( SG_IO, SG_ALERT,
                            "Generic protocol: Received binary "
                            "record of unexpected size, expected: "
                            << binary_record_length << " but received: "
                            << length);
                }
            }
        } else {
            if (!binary_mode) {
                while ((length = io->readline( buf, FG_MAX_MSG_SIZE )) > 0 ) {
                    parse_message_len( length );
                }
            } else {
                while ((length = io->read( buf, binary_record_length )) 
                          == binary_record_length ) {
                    parse_message_len( length );
                }

                if ( length > 0 ) {
                    SG_LOG( SG_IO, SG_ALERT,
                        "Generic protocol: Received binary "
                        "record of unexpected size, expected: "
                        << binary_record_length << " but received: "
                        << length);
                }
            }
        }
    }
    return true;
error_out:
    if (exitOnError) {
        fgOSExit(1);
        return true; // should not get there, but please the compiler
    } else
        return false;
}


// close the channel
bool FGGeneric::close() {
    SGIOChannel *io = get_io_channel();

    if ( ((get_direction() == SG_IO_OUT)||
          (get_direction() == SG_IO_BI))
          && ! postamble.empty() ) {
        if ( ! io->write( postamble.c_str(), postamble.size() ) ) {
            SG_LOG( SG_IO, SG_ALERT, "Error writing postamble." );
            return false;
        }
    }

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}


void
FGGeneric::reinit()
{
    SGPath path = globals->findDataPath("Protocol/" + file_name);
    if (!path.exists()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Couldn't find protocol file for '" << file_name << "'");
        return;
    }
    
    SG_LOG(SG_NETWORK, SG_INFO, "Reading communication protocol from " << path);
   
    SGPropertyNode root;
    try {
        readProperties(path, &root);
    } catch (const sg_exception & ex) {
        SG_LOG(SG_NETWORK, SG_ALERT,
         "Unable to load the protocol configuration file: " << ex.getFormattedMessage() );
         return;
    }

    if (direction == "out") {
        SGPropertyNode *output = root.getNode("generic/output");
        if (output) {
            _out_message.clear();
            if (!read_config(output, _out_message))
            {
                // bad configuration
                return;
            }
        }
    } else if (direction == "in") {
        SGPropertyNode *input = root.getNode("generic/input");
        if (input) {
            _in_message.clear();
            if (!read_config(input, _in_message))
            {
                // bad configuration
                return;
            }
            if (!binary_mode && (line_separator.empty() ||
                *line_separator.rbegin() != '\n')) {

                SG_LOG(SG_IO, SG_WARN,
                    "Warning: Appending newline to line separator in generic input.");
                line_separator.push_back('\n');
            }
        }
    }

    initOk = true;
}


bool
FGGeneric::read_config(SGPropertyNode *root, vector<_serial_prot> &msg)
{
    binary_mode = root->getBoolValue("binary_mode");

    if (!binary_mode) {
        /* These variables specified in the $FG_ROOT/data/Protocol/xxx.xml
         * file for each format
         *
         * var_sep_string  = the string/charachter to place between variables
         * line_sep_string = the string/charachter to place at the end of each
         *                   lot of variables
         */
        preamble = unescape(root->getStringValue("preamble"));
        postamble = unescape(root->getStringValue("postamble"));
        var_sep_string = unescape(root->getStringValue("var_separator"));
        line_sep_string = unescape(root->getStringValue("line_separator"));

        if ( var_sep_string == "newline" ) {
            var_separator = '\n';
        } else if ( var_sep_string == "tab" ) {
            var_separator = '\t';
        } else if ( var_sep_string == "space" ) {
            var_separator = ' ';
        } else if ( var_sep_string == "formfeed" ) {
            var_separator = '\f';
        } else if ( var_sep_string == "carriagereturn" ) {
            var_sep_string = '\r';
        } else if ( var_sep_string == "verticaltab" ) {
            var_separator = '\v';
        } else {
            var_separator = var_sep_string;
        }

        if ( line_sep_string == "newline" ) {
            line_separator = '\n';
        } else if ( line_sep_string == "tab" ) {
            line_separator = '\t';
        } else if ( line_sep_string == "space" ) {
            line_separator = ' ';
        } else if ( line_sep_string == "formfeed" ) {
            line_separator = '\f';
        } else if ( line_sep_string == "carriagereturn" ) {
            line_separator = '\r';
        } else if ( line_sep_string == "verticaltab" ) {
            line_separator = '\v';
        } else {
            line_separator = line_sep_string;
        }
    } else {
        // default values: no footer and record_length = sizeof(representation)
        binary_footer_type = FOOTER_NONE;
        binary_record_length = -1;

        // default choice is network byte order (big endian)
        if (sgIsLittleEndian()) {
           binary_byte_order = BYTE_ORDER_NEEDS_CONVERSION;
        } else {
           binary_byte_order = BYTE_ORDER_MATCHES_NETWORK_ORDER;
        }

        if ( root->hasValue("binary_footer") ) {
            string footer_type = root->getStringValue("binary_footer");
            if ( footer_type == "length" ) {
                binary_footer_type = FOOTER_LENGTH;
            } else if ( footer_type.substr(0, 5) == "magic" ) {
                binary_footer_type = FOOTER_MAGIC;
                binary_footer_value = strtol(footer_type.substr(6, 
                            footer_type.length() - 6).c_str(), (char**)0, 0);
            } else if ( footer_type != "none" ) {
                SG_LOG(SG_IO, SG_ALERT,
                       "generic protocol: Unknown generic binary protocol "
                       "footer '" << footer_type << "', using no footer.");
            }
        }

        if ( root->hasValue("record_length") ) {
            binary_record_length = root->getIntValue("record_length");
        }

        if ( root->hasValue("byte_order") ) {
            string byte_order = root->getStringValue("byte_order");
            if (byte_order == "network" ) {
                if ( sgIsLittleEndian() ) {
                    binary_byte_order = BYTE_ORDER_NEEDS_CONVERSION;
                } else {
                    binary_byte_order = BYTE_ORDER_MATCHES_NETWORK_ORDER;
                }
            } else if ( byte_order == "host" ) {
                binary_byte_order = BYTE_ORDER_MATCHES_NETWORK_ORDER;
            } else {
                SG_LOG(SG_IO, SG_ALERT,
                       "generic protocol: Undefined generic binary protocol"
                       "byte order, using HOST byte order.");
            }
        }

        if( root->hasValue( "wrapper" ) ) {
            string w = root->getStringValue( "wrapper" );
            if( w == "kiss" )  wrapper = new FGKissWrapper();
            else if( w == "stxetx" )  wrapper = new FGSTXETXWrapper();
            else SG_LOG(SG_IO, SG_ALERT,
                       "generic protocol: Undefined binary protocol wrapper '" + w + "' ignored" );
        }
    }

    int record_length = 0; // Only used for binary protocols.
    vector<SGPropertyNode_ptr> chunks = root->getChildren("chunk");

    for (unsigned int i = 0; i < chunks.size(); i++) {

        _serial_prot chunk;

        // chunk.name = chunks[i]->getStringValue("name");
        chunk.format = unescape(chunks[i]->getStringValue("format", "%d"));
        chunk.offset = chunks[i]->getDoubleValue("offset");
        chunk.factor = chunks[i]->getDoubleValue("factor", 1.0);
        chunk.min = chunks[i]->getDoubleValue("min");
        chunk.max = chunks[i]->getDoubleValue("max");
        chunk.wrap = chunks[i]->getBoolValue("wrap");
        chunk.rel = chunks[i]->getBoolValue("relative");

        if( chunks[i]->hasChild("const") ) {
            chunk.prop = new SGPropertyNode();
            chunk.prop->setStringValue( chunks[i]->getStringValue("const", "" ) );
        } else {
            string node = chunks[i]->getStringValue("node", "/null");
            chunk.prop = fgGetNode(node.c_str(), true);
        }

        string type = chunks[i]->getStringValue("type");

        // Note: officially the type is called 'bool' but for backward
        //       compatibility 'boolean' will also be supported.
        if (type == "bool" || type == "boolean") {
            chunk.type = FG_BOOL;
            record_length += 1;
        } else if (type == "float") {
            chunk.type = FG_FLOAT;
            record_length += sizeof(int32_t);
        } else if (type == "double") {
            chunk.type = FG_DOUBLE;
            record_length += sizeof(int64_t);
        } else if (type == "fixed") {
            chunk.type = FG_FIXED;
            record_length += sizeof(int32_t);
        } else if (type == "string") {
            chunk.type = FG_STRING;
        } else if (type == "byte") {
            chunk.type = FG_BYTE;
            record_length += sizeof(int8_t);
        } else if (type == "word") {
            chunk.type = FG_WORD;
            record_length += sizeof(int16_t);
        } else {
            chunk.type = FG_INT;
            record_length += sizeof(int32_t);
        }
        msg.push_back(chunk);

    }

    if( !binary_mode )
    {
        if ((chunks.size() > 1)&&(var_sep_string.length() == 0))
        {
            // ASCII protocols really need a separator when there is more than one chunk per line
            SG_LOG(SG_IO, SG_ALERT,
                   "generic protocol: Invalid configuration. "
                   "'var_separator' must not be empty for protocols which have more than one chunk per line.");
            return false;
        }
    }
    else
    {
        if (binary_record_length == -1) {
            binary_record_length = record_length;
        } else if (binary_record_length < record_length) {
            SG_LOG(SG_IO, SG_ALERT,
                   "generic protocol: Requested binary record length shorter than "
                   " requested record representation.");
            binary_record_length = record_length;
        }
    }

    return true;
}

void FGGeneric::updateValue(FGGeneric::_serial_prot& prot, bool val)
{
  if( prot.rel )
  {
    // value inverted if received true, otherwise leave unchanged
    if( val )
      setValue(prot.prop, !getValue<bool>(prot.prop));
  }
  else
  {
    setValue(prot.prop, val);
  }
}
