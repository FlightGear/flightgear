// FGFSDemo.java - Simple demo application.

import java.io.IOException;

import java.util.HashMap;

import javax.swing.JFrame;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;

import org.flightgear.fgfsclient.FGFSConnection;
import org.flightgear.fgfsclient.PropertyPage;


/**
 * Simple GUI demo.
 *
 * <p>This demo connects to a running FlightGear process and displays
 * the current altitude, longitude, and latitude in a GUI window, with
 * updates every second.</p>
 *
 * <p>Usage:</p>
 *
 * <blockquote><pre>
 * fgfs --telnet=9000
 * java FGFSDemo localhost 9000
 * </pre></blockquote>
 */
public class FGFSDemo
    extends JFrame
{

    public FGFSDemo (String host, int port)
	throws IOException
    {
	super("FlightGear Client Console");

	fgfs = new FGFSConnection(host, port);
	tabs = new JTabbedPane();
	pages = new HashMap();

	PropertyPage page = new PropertyPage(fgfs, "Simulation");
	page.addField("/sim/aircraft", "Aircraft:");
	page.addField("/sim/startup/airport-id", "Airport ID:");
	page.addField("/sim/time/gmt", "Current time (GMT):");
	page.addField("/sim/startup/trim", "Trim on ground (true/false):");
	page.addField("/sim/sound/audible", "Sound enabled (true/false):");
	page.addField("/sim/startup/browser-app", "Web browser:");
	addPage(page);

	page = new PropertyPage(fgfs, "View");
	page.addField("/sim/view-mode", "View mode:");
	page.addField("/sim/current-view/field-of-view",
		      "Field of view (deg):");
	page.addField("/sim/current-view/pitch-offset-deg",
		      "View pitch offset (deg):");
	page.addField("/sim/current-view/heading-offset-deg",
		      "View heading offset (deg):");
	addPage(page);

	page = new PropertyPage(fgfs, "Location");
	page.addField("/position/altitude-ft", "Altitude (ft):");
	page.addField("/position/longitude-deg", "Longitude (deg):");
	page.addField("/position/latitude-deg", "Latitude (deg):");
	page.addField("/orientation/roll-deg", "Roll (deg):");
	page.addField("/orientation/pitch-deg", "Pitch (deg):");
	page.addField("/orientation/heading-deg", "Heading (deg):");
	addPage(page);

	page = new PropertyPage(fgfs, "Weather");
	page.addField("/environment/wind-from-heading-deg",
		      "Wind direction (deg FROM):");
	page.addField("/environment/params/base-wind-speed-kt",
		      "Wind speed (kt):");
	page.addField("/environment/params/gust-wind-speed-kt",
		      "Maximum gust (kt):");
	page.addField("/environment/wind-from-down-fps",
		      "Updraft (fps):");
	page.addField("/environment/temperature-degc", "Temperature (degC):");
	page.addField("/environment/dewpoint-degc", "Dewpoint (degC):");
	page.addField("/environment/pressure-sea-level-inhg",
		      "Altimeter setting (inHG):");
	addPage(page);

	page = new PropertyPage(fgfs, "Clouds");
	page.addField("/environment/clouds/layer[0]/type",
		      "Layer 0 type:");
	page.addField("/environment/clouds/layer[0]/elevation-ft",
		      "Layer 0 height (ft):");
	page.addField("/environment/clouds/layer[0]/thickness-ft",
		      "Layer 0 thickness (ft):");
	page.addField("/environment/clouds/layer[1]/type",
		      "Layer 1 type:");
	page.addField("/environment/clouds/layer[1]/elevation-ft",
		      "Layer 1 height (ft):");
	page.addField("/environment/clouds/layer[1]/thickness-ft",
		      "Layer 1 thickness (ft):");
	page.addField("/environment/clouds/layer[2]/type",
		      "Layer 2 type:");
	page.addField("/environment/clouds/layer[2]/elevation-ft",
		      "Layer 2 height (ft):");
	page.addField("/environment/clouds/layer[2]/thickness-ft",
		      "Layer 2 thickness (ft):");
	page.addField("/environment/clouds/layer[3]/type",
		      "Layer 3 type:");
	page.addField("/environment/clouds/layer[3]/elevation-ft",
		      "Layer 3 height (ft):");
	page.addField("/environment/clouds/layer[3]/thickness-ft",
		      "Layer 3 thickness (ft):");
	page.addField("/environment/clouds/layer[4]/type",
		      "Layer 4 type:");
	page.addField("/environment/clouds/layer[4]/elevation-ft",
		      "Layer 4 height (ft):");
	page.addField("/environment/clouds/layer[4]/thickness-ft",
		      "Layer 4 thickness (ft):");
	addPage(page);

	page = new PropertyPage(fgfs, "Velocities");
	page.addField("/velocities/airspeed-kt", "Airspeed (kt):");
	page.addField("/velocities/speed-down-fps", "Descent speed (fps):");
	addPage(page);

	getContentPane().add(tabs);

	new Thread(new Updater()).start();
    }

    private void addPage (PropertyPage page)
    {
	tabs.add(page.getName(), new JScrollPane(page));
	pages.put(page.getName(), page);
    }

    private FGFSConnection fgfs;
    private JTabbedPane tabs;
    private HashMap pages;

    public static void main (String args[])
	throws Exception
    {
	if (args.length != 2) {
	    System.err.println("Usage: FGFSDemo <host> <port>");
	    System.exit(2);
	}
	FGFSDemo gui = new FGFSDemo(args[0], Integer.parseInt(args[1]));
	gui.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	gui.pack();
	gui.show();
    }

    class Updater
	implements Runnable
    {
	
	public void run ()
	{
	    while (true) {
		int index = tabs.getSelectedIndex();
		if (index > -1) {
		    String name = tabs.getTitleAt(index);
		    PropertyPage page = (PropertyPage)pages.get(name);
		    try {
			page.update();
		    } catch (IOException e) {
		    }
		}
 		try {
 		    Thread.sleep(1000);
 		} catch (InterruptedException e) {
 		}
	    }
	}

    }

}

// end of FGFSDemo.java
