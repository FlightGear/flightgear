package org.flightgear.fgfsclient;

import java.io.IOException;

import java.util.ArrayList;
import java.util.Iterator;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

public class PropertyPage
    extends JPanel
{

    public PropertyPage (FGFSConnection fgfs, String name)
    {
	this.fgfs = fgfs;
	this.name = name;
	fields = new ArrayList();
	setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
    }

    public String getName ()
    {
	return name;
    }

    public void addField (String name, String caption)
    {
	PropertyField field = new PropertyField(fgfs, name, caption);
	add(field);
	fields.add(field);
    }

    public void update ()
	throws IOException
    {
	Iterator it = fields.iterator();
	while (it.hasNext()) {
	    ((PropertyField)it.next()).update();
	}
    }

    private FGFSConnection fgfs;
    private String name;
    private ArrayList fields;

}
