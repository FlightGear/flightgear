TerraSync
=========

Usage:

    terrasync -p <port> [ -s <rsync_source> ] -d <rsync_dest>

Example:

    $ fgfs --atlas=socket,out,1,localhost,5500,udp --fg-scenery=/data1/Scenery-0.7.9
    $ nice terrasync -p 5500 -d /data1/Scenery-0.7.9

Requirements:

    - rsync util installed in your path.
    - mkdir util installed in your path.

TerraSync is a utility that is intended to run as a light weight
background process.  It's purpose is to monitor the position of a
FlightGear flight and pre-fetch scenery tiles from a remote server
based on the current FlightGear position.

This allows you to do a base install of FlightGear with no add on
scenery.  Now just go and fly anywhere.  Scenery is fetched "just in
time" and accumulated on your HD so it is already there next time you
fly.  You can fly anywhere and essentially just the scenery you need
is auto-installed as you fly.

Terrasync runs as a separate process and expects the --atlas=port
format to be sent from fgfs.  The fgfs output tells the terrasync util
where FlightGear is currently flying.  Terrasync will then issue the
appropriate commands to rsync the surrounding areas to your local
scenery directory.  The user need to choose a port for
FlightGear->TerraSync communication and then specify the server
location and destation scenery tree.

As you fly, terrasync will periodically refresh and pull any new
scenery tiles that you need for your position from the server.

This also works if the scenery on the scenery server is updated.
Rsync will pull any missing files, or any updated files.

There is a chicken/egg problem when you first start up in a brand new
area.  FlightGear is expecting the scenery to be there *now* but it
may not have been fetched yet.  I suppose without making a more
complex protocol, the user will need to be aware of this.  The user
could restart flightgear after the initial rsync completes, and then
after that everything should be good, assuming the flight track is
"continuous" and the user has the necessary bandwidth to keep up with
flight speeds.

Final notes:

I have set up an initial scenery server at
baron.flightgear.org::Scenery-0.7.9.  This is the 0.7.9 vintage
scenery with airports rebuilt to include lighting.  Alex Perry also
has a partial rsync server, but I don't know it's current status.
William Riley has rebuilt the entire world, but the tiles are zipped
in 10x10 degree chunks on on a relatively low bandwidth connection.
There are aspects of the 0.7.9 scenery I like including the quality of
the terrain, but William scenery has roads and streams which is also
nice.
