// FGFSConnection.java - client library for the FlightGear flight simulator.
// Started June 2002 by David Megginson, david@megginson.com
// This library is in the Public Domain and comes with NO WARRANTY.

package org.flightgear.fgfsclient;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.PrintWriter;

import java.net.Socket;


/**
 * A connection to a running instance of FlightGear.
 *
 * <p>This class currently uses the FlightGear telnet interface,
 * though it may be modified to use a different TCP/IP interface in
 * the future.  Client applications can use this library to examine
 * and modify internal FlightGear properties.</p>
 *
 * <p>To start FlightGear with the telnet server activated, use a
 * command like this (to listen on port 9000):</p>
 *
 * <blockquote><pre>
 * fgfs --telnet=9000
 * </pre></blockquote>
 *
 * <p>Then create a connection to FlightGear from your Java client
 * application:</p>
 *
 * <blockquote><pre>
 * FGFSConnection fgfs = new FGFSConnection("localhost", 9000);
 * </pre></blockquote>
 *
 * <p>Now you can use the connection to get and set FlightGear
 * properties:</p>
 *
 * <blockquote><pre>
 * double altitude = fgfs.getDouble("/position/altitude-ft");
 * fgfs.setDouble("/orientation/heading", 270.0);
 * </pre></blockquote>
 *
 * <p>All methods that communicate directly with FlightGear are
 * synchronized, since they must work over a single telnet
 * connection.</p>
 */
public class FGFSConnection
{


    ////////////////////////////////////////////////////////////////////
    // Constructor.
    ////////////////////////////////////////////////////////////////////


    /**
     * Constructor.
     *
     * <p>Create a new connection to a running FlightGear program.
     * The program must have been started with the --telnet=&lt;port&gt;
     * command-line option.</p>
     *
     * @param host The host name or IP address to connect to.
     * @param port The port number where FlightGear is listening.
     * @exception IOException If it is not possible to connect to
     * a FlightGear process.
     */
    public FGFSConnection (String host, int port)
	throws IOException
    {
	socket = new Socket(host, port);
	in =
	    new BufferedReader(new InputStreamReader(socket.getInputStream()));
	out = new PrintWriter(socket.getOutputStream(), true);
	out.println("data\r");
    }



    ////////////////////////////////////////////////////////////////////
    // Primitive getter and setter.
    ////////////////////////////////////////////////////////////////////


    /**
     * Close the connection to FlightGear.
     *
     * <p>The client application should always invoke this method when
     * it has finished with a connection, to allow cleanup.</p>
     *
     * @exception IOException If there is an error closing the
     * connection.
     */
    public synchronized void close ()
	throws IOException
    {
	out.println("quit\r");
	in.close();
	out.close();
	socket.close();
    }


    /**
     * Get the raw string value for a property.
     *
     * <p>This is the primitive method for all property lookup;
     * everything comes in as a string, and is only later converted by
     * methods like {@link #getDouble(String)}.  As a result, if you
     * need the value as a string anyway, it makes sense to use this
     * method directly rather than forcing extra conversions.</p>
     *
     * @param name The FlightGear property name to look up.
     * @return The property value as a string (non-existant properties
     * return the empty string).
     * @exception IOException If there is an error communicating with
     * FlightGear or if the connection is lost.
     * @see #getBoolean(String)
     * @see #getInt(String)
     * @see #getLong(String)
     * @see #getFloat(String)
     * @see #getDouble(String)
     */
    public synchronized String get (String name)
	throws IOException
    {
	out.println("get " + name + '\r');
	return in.readLine();
    }


    /**
     * Set the raw string value for a property.
     *
     * <p>This is the primitive method for all property modification;
     * everything goes out as a string, after it has been converted by
     * methods like {@link #setDouble(String,double)}.  As a result, if
     * you have the value as a string already, it makes sense to use
     * this method directly rather than forcing extra conversions.</p>
     *
     * @param name The FlightGear property name to modify or create.
     * @param value The new value for the property, as a string.
     * @exception IOException If there is an error communicating with
     * FlightGear or if the connection is lost.
     * @see #setBoolean(String,boolean)
     * @see #setInt(String,int)
     * @see #setLong(String,long)
     * @see #setFloat(String,float)
     * @see #setDouble(String,double)
     */
    public synchronized void set (String name, String value)
	throws IOException
    {
	out.println("set " + name + ' ' + value + '\r');
    }



    ////////////////////////////////////////////////////////////////////
    // Derived getters and setters.
    ////////////////////////////////////////////////////////////////////


    /**
     * Get a property value as a boolean.
     *
     * @param name The property name to look up.
     * @return The property value as a boolean.
     * @see #get(String)
     */
    public boolean getBoolean (String name)
	throws IOException
    {
	return get(name).equals("true");
    }


    /**
     * Get a property value as an integer.
     *
     * @param name The property name to look up.
     * @return The property value as an int.
     * @see #get(String)
     */
    public int getInt (String name)
	throws IOException
    {
	return Integer.parseInt(get(name));
    }


    /**
     * Get a property value as a long.
     *
     * @param name The property name to look up.
     * @return The property value as a long.
     * @see #get(String)
     */
    public long getLong (String name)
	throws IOException
    {
	return Long.parseLong(get(name));
    }


    /**
     * Get a property value as a float.
     *
     * @param name The property name to look up.
     * @return The property value as a float.
     * @see #get(String)
     */
    public float getFloat (String name)
	throws IOException
    {
	return Float.parseFloat(get(name));
    }


    /**
     * Get a property value as a double.
     *
     * @param name The property name to look up.
     * @return The property value as a double.
     * @see #get(String)
     */
    public double getDouble (String name)
	throws IOException
    {
	return Double.parseDouble(get(name));
    }


    /**
     * Set a property value from a boolean.
     *
     * @param name The property name to create or modify.
     * @param value The new property value as a boolean.
     * @see #set(String,String)
     */
    public void setBoolean (String name, boolean value)
	throws IOException
    {
	set(name, (value ? "true" : "false"));
    }


    /**
     * Set a property value from an int.
     *
     * @param name The property name to create or modify.
     * @param value The new property value as an int.
     * @see #set(String,String)
     */
    public void setInt (String name, int value)
	throws IOException
    {
	set(name, Integer.toString(value));
    }


    /**
     * Set a property value from a long.
     *
     * @param name The property name to create or modify.
     * @param value The new property value as a long.
     * @see #set(String,String)
     */
    public void setLong (String name, long value)
	throws IOException
    {
	set(name, Long.toString(value));
    }


    /**
     * Set a property value from a float.
     *
     * @param name The property name to create or modify.
     * @param value The new property value as a float.
     * @see #set(String,String)
     */
    public void setFloat (String name, float value)
	throws IOException
    {
	set(name, Float.toString(value));
    }


    /**
     * Set a property value from a double.
     *
     * @param name The property name to create or modify.
     * @param value The new property value as a double.
     * @see #set(String,String)
     */
    public void setDouble (String name, double value)
	throws IOException
    {
	set(name, Double.toString(value));
    }



    ////////////////////////////////////////////////////////////////////
    // Internal state.
    ////////////////////////////////////////////////////////////////////

    private Socket socket;
    private BufferedReader in;
    private PrintWriter out;

}

// end of FGFSConnection.java
