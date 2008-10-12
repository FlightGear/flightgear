TerraSync
=========

Usage:
	
    terrasync -p <port> [ -R ] [ -s <rsync_source> ] -d <dest>
    terrasync -p <port>   -S   [ -s <svn_source> ] -d <dest>

Example:

    $ fgfs --atlas=socket,out,1,localhost,5500,udp --fg-scenery=/usr/local/FlightGear/data/scenery:/data1/Scenery-0.9.2
    $ nice terrasync -p 5500 -d /data1/Scenery-0.9.2

Requirements:

    - rsync util installed in your path.
or	- svn util installed in your path

NOTE!!!!  Do not run terrasync against the default
$FG_ROOT/data/Scenery/ directory.  I recommend creating a separate
scenery directory and specifying both in your ~/.fgfsrc file or via
the command line.

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
Rsync ( or svn ) will pull any missing files, or any newly updated files.

There is a chicken/egg problem when you first start up in a brand new
area.  FlightGear is expecting the scenery to be there *now* but it
may not have been fetched yet.  I suppose without making a more
complex protocol, the user will need to be aware of this.  For now I
suggest exiting FlightGear after terrasync gets caught up, and
restarting FlightGear.  The second time FlightGear starts up
everything should be good.

Final notes:

I have set up the scenery server at
scenery.flightgear.org::Scenery-0.9.2.  This is the latest 0.9.2 build
of the world.

Note to self:

nice ./terrasync -p 5500 -s baron.flightgear.org:/stage/fgfs05/curt/Scenery-1.0 -d /stage/catalina3/Scenery-1.0/
