David Calkins writes:

I've attached my sample code which works with FlightGear v0.9.3.  Perhaps this
will be of some help to others.  I'm running FG with the launcher wizard, which
uses the below command line options.  The sample code I attached just rolls
back and forth ± 5 degrees so it isn't that interesting, but it works.

C:\Program Files\FlightGear-0.9.3\bin\Win32\fgfs.exe
  --fg-root=C:\Program Files\FlightGear-0.9.3\data
  --fg-scenery=C:\Program Files\FlightGear-0.9.3\data\Scenery
  --aircraft=c172
  --control=joystick
  --disable-random-objects
  --fdm=external
  --vc=0
  --bpp=32
  --timeofday=noon
  --native-fdm=socket,in,1,,5500,udp

One point of interest is the cur_time field in the FGNetFDM structure.
I noticed that it didn't seem to matter what time I passed in.  In looking at
the source, it appears this field is ignored completely.  So, it looks like the
time of day would need to be determined by the command line parameters used to
launch FlightGear.

Dave 
