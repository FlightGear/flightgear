from telnetlib import Telnet
import sys
import socket
import re
from string import split, join
import time

__all__ = ["FlightGear"]

CRLF = '\r\n'

class FGTelnet(Telnet):
    def __init__(self,host,port):
        Telnet.__init__(self,host,port)
        self.prompt = []
        self.prompt.append( re.compile('/[^>]*> ') )
        self.timeout = 5
        #Telnet.set_debuglevel(self,2)

    def help(self):
        return

    def ls(self,dir=None):
        """
        Returns a list of properties.
        """
        if dir == None:
            self._putcmd('ls')
        else:
            self._putcmd('ls %s' % dir )
        return self._getresp()

    def dump(self):
        """Dump current state as XML."""
        self._putcmd('dump')
        return self._getresp()

    def cd(self, dir):
        """Change directory."""
        self._putcmd('cd ' + dir)
        self._getresp()
        return

    def pwd(self):
        """Display current path."""
        self._putcmd('pwd')
        return self._getresp()

    def get(self,var):
        """Retrieve the value of a parameter."""
        self._putcmd('get %s' % var )
        return self._getresp()

    def set(self,var,value):
        """Set variable to a new value"""
        self._putcmd('set %s %s' % (var,value))
        self._getresp() # Discard response

    def quit(self):
        """Terminate connection"""
        self._putcmd('quit')
        self.close()
        return

    # Internal: send one command to FlightGear
    def _putcmd(self,cmd):
        cmd = cmd + CRLF;
        Telnet.write(self, cmd)
        return

    # Internal: get a response from FlightGear
    def _getresp(self):
        (i,match,resp) = Telnet.expect(self, self.prompt, self.timeout)
        # Remove the terminating prompt.
        # Everything preceding it is the response.
        return split(resp, '\n')[:-1]

class FlightGear:
    """FlightGear interface class.

    An instance of this class represents a connection to a FlightGear telnet
    server.

    Properties are accessed using a dictionary style interface:
    For example:

    # Connect to flightgear telnet server.
    fg = FlightGear('myhost', 5500)
    # parking brake on
    fg['/controls/gear/brake-parking'] = 1
    # Get current heading
    heading = fg['/orientation/heading-deg']

    Other non-property related methods
    """

    def __init__( self, host = 'localhost', port = 5500 ):
        try:
            self.telnet = FGTelnet(host,port)
        except socket.error, msg:
            self.telnet = None
            raise socket.error, msg

    def __del__(self):
        # Ensure telnet connection is closed cleanly.
        self.quit();

    def __getitem__(self,key):
        """Get a FlightGear property value.
        Where possible the value is converted to the equivalent Python type.
        """
        s = self.telnet.get(key)[0]
        match = re.compile( '[^=]*=\s*\'([^\']*)\'\s*([^\r]*)\r').match( s )
        if not match:
            return None
        value,type = match.groups()
        #value = match.group(1)
        #type = match.group(2)
        if value == '':
            return None

        if type == '(double)':
            return float(value)
        elif type == '(int)':
            return int(value)
        elif type == '(bool)':
            if value == 'true':
                return 1
            else:
                return 0
        else:
            return value

    def __setitem__(self, key, value):
        """Set a FlightGear property value."""
        self.telnet.set( key, value )

    def quit(self):
        """Close the telnet connection to FlightGear."""
        if self.telnet:
            self.telnet.quit()
            self.telnet = None

    def view_next(self):
        #move to next view
        self.telnet.set( "/command/view/next", "true")

    def view_prev(self):
        #move to next view
        self.telnet.set( "/command/view/prev", "true")

