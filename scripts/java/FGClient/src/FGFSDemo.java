// FGFSDemo.java - Simple demo application.

import java.io.IOException;

import java.awt.FlowLayout;

import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JTextField;

import org.flightgear.fgfsclient.FGFSConnection;


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

	getContentPane().setLayout(new FlowLayout());

	altitudeLabel = new JTextField(fgfs.get("/position/altitude-ft"));
	longitudeLabel = new JTextField(fgfs.get("/position/longitude-deg"));
	latitudeLabel = new JTextField(fgfs.get("/position/latitude-deg"));

	getContentPane().add(new JLabel("Altitude: "));
	getContentPane().add(altitudeLabel);
	getContentPane().add(new JLabel("Longitude: "));
	getContentPane().add(longitudeLabel);
	getContentPane().add(new JLabel("Latitude: "));
	getContentPane().add(latitudeLabel);

	new Thread(new Updater()).start();
    }

    private FGFSConnection fgfs;
    private JTextField altitudeLabel;
    private JTextField longitudeLabel;
    private JTextField latitudeLabel;

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
		try {
		    altitudeLabel.setText(fgfs.get("/position/altitude-ft"));
		    longitudeLabel.setText(fgfs.get("/position/longitude-deg"));
		    latitudeLabel.setText(fgfs.get("/position/latitude-deg"));
		} catch (IOException e) {
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
