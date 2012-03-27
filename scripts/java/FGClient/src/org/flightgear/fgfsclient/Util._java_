package org.flightgear.fgfsclient;

import java.io.IOException;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JTextField;


public final class Util
{

    public static void forceConnection (JFrame frame, final FGFSConnection fgfs)
    {
	while (!fgfs.isConnected()) {
	    final JDialog dialog =
		new JDialog(frame, "Connect to FlightGear", true);
	    dialog.getContentPane().setLayout(new GridLayout(3, 2));
	    dialog.getContentPane().add(new JLabel("Host:"));

	    final JTextField hostField = new JTextField(20);
	    hostField.setText(fgfs.getHost());
	    dialog.getContentPane().add(hostField);

	    dialog.getContentPane().add(new JLabel("Port:"));

	    final JTextField portField = new JTextField(5);
	    portField.setText(Integer.toString(fgfs.getPort()));
	    dialog.getContentPane().add(portField);

	    JButton connectButton = new JButton("Connect");
	    connectButton.addActionListener(new ActionListener() {
		public void actionPerformed (ActionEvent ev)
		{
		    try {
			fgfs.open(hostField.getText(),
				  Integer.parseInt(portField.getText()));
		    } catch (IOException ex) {
			JOptionPane.showMessageDialog(dialog, ex.getMessage(),
						      "Alert", JOptionPane.ERROR_MESSAGE);
		    }
		    dialog.hide();
		}
	    });
	    dialog.getContentPane().add(connectButton);
	    dialog.getRootPane().setDefaultButton(connectButton);

	    JButton quitButton = new JButton("Quit");
	    quitButton.addActionListener(new ActionListener() {
		public void actionPerformed (ActionEvent ev)
		{
		    System.exit(0); // FIXME
		}
	    });
	    dialog.getContentPane().add(quitButton);

	    dialog.pack();
	    dialog.show();
	    
	}
    }

}
