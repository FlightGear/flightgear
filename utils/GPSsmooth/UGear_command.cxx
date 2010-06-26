#include <cstring>
#include <cstdio>

#include "UGear_command.hxx"


UGCommand::UGCommand():
    cmd_send_index(0),
    cmd_recv_index(0),
    prime_state(true)
{}


UGCommand::~UGCommand() {}


// calculate the nmea check sum
static char calc_nmea_cksum(const char *sentence) {
    unsigned char sum = 0;
    int i, len;

    // cout << sentence << endl;

    len = std::strlen(sentence);
    sum = sentence[0];
    for ( i = 1; i < len; i++ ) {
        // cout << sentence[i];
        sum ^= sentence[i];
    }
    // cout << endl;

    // printf("sum = %02x\n", sum);
    return sum;
}


// package and send the serial command
static int serial_send( SGSerialPort *serial, int sequence,
                        const string command )
{
    char sequence_str[10];
    snprintf( sequence_str, 9, "%d", sequence );

    string package = sequence_str;
    package += ",";
    package += command;

    char pkg_sum[10];
    snprintf( pkg_sum, 3, "%02X", calc_nmea_cksum(package.c_str()) );

    package += "*";
    package += pkg_sum;
    package += "\n";

    unsigned int result = serial->write_port( package.c_str(),
                                              package.length() );
    if ( result != package.length() ) {
        printf("ERROR: wrote %u of %u bytes to serial port!\n",
               result, (unsigned)package.length() );
        return 0;
    }

    return 1;
}


// send current command until acknowledged
int UGCommand::update( SGSerialPort *serial )
{
    // if current command has been received, advance to next command
    printf("sent = %d  recv = %d\n", cmd_send_index, cmd_recv_index);
    if ( cmd_recv_index >= cmd_send_index ) {
        if ( ! cmd_queue.empty() ) {
            if ( ! prime_state ) {
                cmd_queue.pop();
                cmd_send_index++;
            } else {
                prime_state = false;
            }
        }
    }

    // nothing to do if command queue empty
    if ( cmd_queue.empty() ) {
        prime_state = true;
        return 0;
    }

    // send the command
    string command = cmd_queue.front();
    /*int result =*/ serial_send( serial, cmd_send_index, command );

    return cmd_send_index;
}


void UGCommand::add( const string command )
{
    printf("command queue: %s\n", command.c_str());
    cmd_queue.push( command );
}


// create the global command channel manager
UGCommand command_mgr;

