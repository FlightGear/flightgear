import Tix
from Tkconstants import *
from FlightGear import FlightGear

import sys
import socket

class PropertyField:
    def __init__(self, parent, prop, label):
        self.prop = prop
        self.field = Tix.LabelEntry( parent, label=label,
             options='''
             label.width 30
             label.anchor e
             entry.width 30
             ''' )
        self.field.pack( side=Tix.TOP, padx=20, pady=2 )

    def update_field(self,fgfs):
        val = fgfs[self.prop]
        self.field.entry.delete(0,'end')
        self.field.entry.insert(0, val)
    
class PropertyPage(Tix.Frame):
    def __init__(self,parent,fgfs):
        Tix.Frame.__init__(self,parent)
        self.fgfs = fgfs
        self.pack( side=Tix.TOP, padx=2, pady=2, fill=Tix.BOTH, expand=1 )
        self.fields = []
        
    def addField(self, prop, label):
        f = PropertyField(self, prop, label)
        self.fields.append(f)

    def update_fields(self):
        for f in self.fields:
            f.update_field(self.fgfs)
            Tix.Frame.update(self)

class FGFSDemo(Tix.Frame):
    def __init__(self,fgfs,root=None):
        Tix.Frame.__init__(self,root)
        z = root.winfo_toplevel()
        z.wm_protocol("WM_DELETE_WINDOW", lambda self=self: self.quitcmd())
        self.fgfs = fgfs
        self.pack()
        self.pages = {}
        self.after_id = None
        self.createWidgets()
        self.update()

    def createWidgets(self):
        self.nb = Tix.NoteBook(self)
        self.nb.add( 'sim', label='Simulation',
                     raisecmd= lambda self=self: self.update_page() )
        self.nb.add( 'view', label='View',
                     raisecmd= lambda self=self: self.update_page() )
        self.nb.add( 'loc', label='Location',
                     raisecmd= lambda self=self: self.update_page() )
        self.nb.add( 'weather', label='Weather',
                     raisecmd= lambda self=self: self.update_page() )
        self.nb.add( 'clouds', label='Clouds',
                     raisecmd= lambda self=self: self.update_page() )
        self.nb.add( 'velocities', label='Velocities',
                     raisecmd= lambda self=self: self.update_page() )

        page = PropertyPage( self.nb.sim, self.fgfs )
        self.pages['sim'] = page
        page.addField( '/sim/aircraft', 'Aircraft:')
        page.addField( '/sim/startup/airport-id', 'Airport ID:')
        page.addField( '/sim/time/gmt', 'Current time (GMT):')
        page.addField( '/sim/startup/trim', 'Trim on ground (true/false):')
        page.addField( '/sim/sound/audible', 'Sound enabled (true/false):')
        page.addField( '/sim/startup/browser-app', 'Web browser:')

        page = PropertyPage( self.nb.view, self.fgfs )
        self.pages['view'] = page
        page.addField( '/sim/view-mode', 'View mode:')
        page.addField( "/sim/current-view/field-of-view", "Field of view (deg):" )
        page.addField( "/sim/current-view/pitch-offset-deg", "View pitch offset (deg):" )
        page.addField( "/sim/current-view/heading-offset-deg", "View heading offset (deg):" )

        page = PropertyPage( self.nb.loc, self.fgfs )
        self.pages['loc'] = page
        page.addField( "/position/altitude-ft", "Altitude (ft):" )
        page.addField( "/position/longitude-deg", "Longitude (deg):" )
        page.addField( "/position/latitude-deg", "Latitude (deg):" )
        page.addField( "/orientation/roll-deg", "Roll (deg):" )
        page.addField( "/orientation/pitch-deg", "Pitch (deg):" )
        page.addField( "/orientation/heading-deg", "Heading (deg):" )

        page = PropertyPage( self.nb.weather, self.fgfs )
        self.pages['weather'] = page
        page.addField("/environment/wind-from-heading-deg",
                      "Wind direction (deg FROM):")
        page.addField("/environment/params/base-wind-speed-kt",
                      "Wind speed (kt):")
        page.addField("/environment/params/gust-wind-speed-kt",
                      "Maximum gust (kt):")
        page.addField("/environment/wind-from-down-fps",
                      "Updraft (fps):")
        page.addField("/environment/temperature-degc", "Temperature (degC):")
        page.addField("/environment/dewpoint-degc", "Dewpoint (degC):")
        page.addField("/environment/pressure-sea-level-inhg",
                      "Altimeter setting (inHG):")

        page = PropertyPage( self.nb.clouds, self.fgfs )
        self.pages['clouds'] = page
        page.addField("/environment/clouds/layer[0]/type",
                      "Layer 0 type:")
        page.addField("/environment/clouds/layer[0]/elevation-ft",
                      "Layer 0 height (ft):")
        page.addField("/environment/clouds/layer[0]/thickness-ft",
                      "Layer 0 thickness (ft):")
        page.addField("/environment/clouds/layer[1]/type",
                      "Layer 1 type:")
        page.addField("/environment/clouds/layer[1]/elevation-ft",
                      "Layer 1 height (ft):")
        page.addField("/environment/clouds/layer[1]/thickness-ft",
                      "Layer 1 thickness (ft):")
        page.addField("/environment/clouds/layer[2]/type",
                      "Layer 2 type:")
        page.addField("/environment/clouds/layer[2]/elevation-ft",
                      "Layer 2 height (ft):")
        page.addField("/environment/clouds/layer[2]/thickness-ft",
                      "Layer 2 thickness (ft):")
        page.addField("/environment/clouds/layer[3]/type",
                      "Layer 3 type:")
        page.addField("/environment/clouds/layer[3]/elevation-ft",
                      "Layer 3 height (ft):")
        page.addField("/environment/clouds/layer[3]/thickness-ft",
                      "Layer 3 thickness (ft):")
        page.addField("/environment/clouds/layer[4]/type",
                      "Layer 4 type:")
        page.addField("/environment/clouds/layer[4]/elevation-ft",
                      "Layer 4 height (ft):")
        page.addField("/environment/clouds/layer[4]/thickness-ft",
                      "Layer 4 thickness (ft):")

        page = PropertyPage( self.nb.velocities, self.fgfs )
        self.pages['velocities'] = page
        page.addField("/velocities/airspeed-kt", "Airspeed (kt):")
        page.addField("/velocities/speed-down-fps", "Descent speed (fps):")

        self.nb.pack( expand=1, fill=Tix.BOTH, padx=5, pady=5, side=Tix.TOP )
        
        self.QUIT = Tix.Button(self)
        self.QUIT['text'] = 'Quit'
        self.QUIT['command'] = self.quitcmd
        self.QUIT.pack({"side": "bottom"})

    def quitcmd(self):
        if self.after_id:
            self.after_cancel(self.after_id)
        #self.quit()
        self.destroy()

    def update_page(self):
        page = self.pages[ self.nb.raised() ]
        page.update_fields()
        self.update()
        self.after_id = self.after( 1000, lambda self=self: self.update_page() )

def main():
    if len(sys.argv) != 3:
        print 'Usage: %s host port' % sys.argv[0]
        sys.exit(1)

    host = sys.argv[1]
    try:
        port = int( sys.argv[2] )
    except ValueError, msg:
        print 'Error: expected a number for port'
        sys.exit(1)
    
    fgfs = None
    try:
        fgfs = FlightGear( host, port )
    except socket.error, msg:
        print 'Error connecting to flightgear:', msg[1]
        sys.exit(1)
    
    root = Tix.Tk()
    app = FGFSDemo( fgfs, root )
    app.mainloop()

if __name__ == '__main__':
    main()

