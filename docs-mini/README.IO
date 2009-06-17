This document describes how to invoke FlightGear's generic IO subsystem.

FlightGear has a fairly flexible generic IO subsystem that allows you
to "speak" any supported protocol over any supported medium.  The IO
options are configured at runtime via command line options.  You can
specify multiple entries if you like, one per command line option.


The general form of the command line option is as follows:

    --protocol=medium,direction,hz,medium_options,...

    protocol = { native, nmea, garmin, fgfs, rul, pve, ray, etc. }
    medium = { serial, socket, file, etc. }
    direction = { in, out, bi }
    hz = number of times to process channel per second (floating
         point values are ok.


Generic Communication:

    --generic=params

    With this option it is possible to output a pre-configured
    ASCII string or binary sequence  using a predefined separator.
    The configuration is defined in an XML file located in the
    Protocol directory of the base package.

    params can be:
    serial port communication:    serial,dir,hz,device,baud,protocol
    socket communication:         socket,dir,hz,machine,port,style,protocol
    i/o to a file:                file,dir,hz,filename,protocol

    See README.protocol for how to define a generic protocol.


Serial Port Communication:

    --nmea=serial,dir,hz,device,baud

    device = OS device name of serial line to be open()'ed
    baud = {300, 1200, 2400, ..., 230400}

    example to pretend we are a real gps and output to a moving map application:

    --nmea=serial,out,0.5,COM1,4800

    Note that for unix variants you might use a device name like "/dev/ttyS0"


Socket Communication:

    --native=socket,dir,hz,machine,port,style

    machine = machine name or ip address if client (leave empty if server)
    port = port, leave empty to let system choose
    style = tcp or udp

    example to slave one copy of fgfs to another

    fgfs1:  --native=socket,out,30,fgfs2,5500,udp
    fgfs2:  --native=socket,in,30,,5500,udp --fdm=external

    This instructs the first copy of fgfs to send UDP packets in the
    native format to a machine called fgfs2 on port 5500.

    The second copy of fgfs will accept UDP packets (from anywhere) on
    port 5500.  Note the additional --fdm=external option.  This tells
    the second copy of fgfs to not run the normal flight model, but
    instead set the FDM values based on an external source (the
    network in this case.)


File I/O:

    --garmin=file,dir,hz,filename

    filename = file system file name

    example to record a flight path at 10 hz:

    --native=file,out,10,flight1.fgfs

    example to replay your flight

    --native=file,in,10,flight1.fgfs --fdm=external

    You can make the replay from a file loop back to the beginning
    when it reaches the end of the file with the "repeat" flag:

    --generic=file,in,20,flight.out,playback,repeat

    With a numeric argument, FlightGear will exit after that number of repeats.
    --generic=file,in,20,flight.out,playback,repeat,5


Moving Map Example:

    Per Liedman has developed a moving map program called Atlas
    (atlas.sourceforge.net) The initial inspiration and much code came
    from Alexei Novikov.

    The moving map supports NMEA format input either via network or
    via serial port.  Either way will work, but this example
    demonstrates the use of a socket connection.

    Start up fgfs with:

        fgfs --nmea=socket,out,0.5,atas-host-name,5500,udp

    Start up the Atlas program with:

        Atlas --udp=5500 --fgroot=path-to-fg-root --glutfonts

    Once both programs are running, the Atlas program should display
    your current location.  Atlas is a really nifty program with many
    neat options such as the ability to generate and use background
    bitmaps that show the terrain, cities, lakes, oceans, rivers, etc.


HTTP Server Example

    You can now interact with a running copy of FlightGear using your
    web browser.  You can view all the key internal variables and even
    change the ones that are writable.  If you have support in your
    favorite [scripting] language for interacting with an http server,
    you should be able to use this as a mechanism to interface your
    script with FlightGear.

    Start up fgfs with the --httpd=<port#> option:

    For example:

        fgfs --httpd=5500

    Now point your web browser to:

        http://host.domain.name:5500/

    When a value is displayed, you can click on it to bring up a form
    to assign it a new value.


ACMS flight data recorder playback

  fgfs --fdm=acms --generic=file,in,1,<path_to_replay_file>,acms

