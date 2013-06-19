
Tcl interface to the iax2 client lib.
It links statically to the iaxclient library. 

Copyright (c) 2006 Mats Bengtsson
Copyright (c) 2006 Antonio Cano damas

BSD-style license

Building:

You first need to build the libiaxclient in iaxclient/lib where there
are a number of methods to build on various platforms. This is how I did:

MacOSX:  ProjectBuilder project
Linux:   make
Windows: Dev-C++ (Bloodshed) project

Then you build the tcl package:

MacOSX:  ProjectBuilder project 
Linux:   ./configure
         make
Windows: Dev-C++ (Bloodshed) project

The configure/make method is a generic way to build tcl extensions
on all platforms, but I only tested on Linux.
You may need to experiment with the:
USE_PA_OSS=1
USE_PA_ALSA=1
USE_PA_JACK=1
in the iaxclient/lib/Makefile and don't forget to include relevant link 
libraries when building the tcl package:
TEA_ADD_LIBS([-lpthread -lasound])
etc. in configure.in. Don't forget to run 'autoconf' after you have edit this 
file.


Usage:

    iaxclient::answer
        answer call

    iaxclient::callerid name num
        sets caller id

    iaxclient::changeline line
        changes line

    iaxclient::devices input|output|ring ?-current?
        returns a list {name deviceID} if -current
	else lists all devices as {{name deviceID} ...}

    iaxclient::dial user:pass@server/nnn
        dials client

    iaxclient::formats codec
    
    iaxclient::getport
        returns the listening port number

    iaxclient::hangup
        hang up current call

    iaxclient::hold

    iaxclient::info
        not implemented

    iaxclient::level input|output ?level?
        sets or gets the respective levels with 0 < level < 1

    iaxclient::notify eventType ?tclProc?
        sets or gets a notfier procedure for an event type.
	The valid types and callback forms are:
	<Text>          'procName type callNo message'
	<Levels>        'procName in out'
	<State>         'procName callNo state format remote remote_name local local_context'	               
	<NetStats>      'procName args' where args is a list of '-key value' pairs
	<Url>
	<Video>
	<Registration>  'procName id reply msgcount'
	                reply : ack, rej, timeout

	It returns the present tclProc if any. If you specify an empty
	tclProc the callback will be removed.

    iaxclient::playtone tone

    iaxclient::register username password hostname
        Returns the session id.

    iaxclient::reject
        reject current call

    iaxclient::sendtext text

    iaxclient::sendtone tone

    iaxclient::state

    iaxclient::transfer

    iaxclient::unhold

    iaxclient::unregister sessionID


A tone is any single character from the set 123A456B789C*0#D

A state is a list with any of: free, active, outgoing, ringing, complete, 
selected, busy, or transfer.

A codec is any of G723_1, GSM, ULAW, ALAW, G726, ADPCM, SLINEAR, LPC10,
G729A, SPEEX, or ILBC


