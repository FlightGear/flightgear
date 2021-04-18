from telnetlib import Telnet
import sys
import socket
import re
import time

__all__ = ["FlightGear"]

CRLF = '\r\n'

class FGTelnet(Telnet):
    def __init__(self,host,port):
        Telnet.__init__(self,host,port)
        self.prompt = [re.compile('/[^>]*> '.encode('utf-8'))]
        self.timeout = 5
        #Telnet.set_debuglevel(self,2)

    def help(self):
        return

    def ls(self,dir=None):
        """
        Returns a list of properties.
        """
        if dir is None:
            self._putcmd('ls')
        else:
            self._putcmd('ls %s' % dir )
        return self._getresp()
    
    def ls2(self, dir_):
        self._putcmd(f'ls2 {dir_}')
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
        cmd = cmd + CRLF
        Telnet.write(self, cmd.encode('utf-8'))
        return

    def _getresp(self):
        # Telnet.expect() can return short result, so we call it in a loop.
        response = b''
        while 1:
            _i, _match, data = Telnet.expect(self, self.prompt, self.timeout)
            response += data
            if _i == 0:
                break   # We have the prompt that marks the end of the data.
            assert _i == -1, f'i={i}'
        # Remove the terminating prompt.
        # Everything preceding it is the response.
        return response.decode('utf-8').split('\n')[:-1]

class LsItem:
    def __init__(self, num_children, name, index, type_, value_text):
        self.num_children = num_children
        self.name = name
        self.index = index
        self.type_ = type_
        self.value_text = value_text
        # Convert to correct type; type_ is originally from
        # flightgear/src/Network/props.cxx:getValueTypeString().
        #
        if type_ in ('unknown', 'unspecified', 'none'):
            value = value_text
        elif type_ == 'bool':
            value = (value_text == 'true')
        elif type_ in ('int', 'long'):
            value = int(value_text)
        elif type_ in ('float', 'double'):
            self.value = float(value_text)
        elif type_ == 'string':
            self.value = value_text
        else:
            assert 0, f'Unrecognised type: {type_}'
    
    def __str__(self):
        return f'num_children={self.num_children} name={self.name}[{self.index}] type={self.type_}: {self.value!r}'

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
        except socket.error as msg:
            self.telnet = None
            raise msg

    def __del__(self):
        # Ensure telnet connection is closed cleanly.
        self.quit()

    def __getitem__(self,key):
        """Get a FlightGear property value.
        Where possible the value is converted to the equivalent Python type.
        """
        s = self.telnet.get(key)[0]
        match = re.compile( r'[^=]*=\s*\'([^\']*)\'\s*([^\r]*)\r').match( s )
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
        if value is True:
            # Flightgear props doesn't treat string 'True' as true - see
            # SGPropertyNode::setStringValue().
            value = 'true'
        self.telnet.set( key, value )
    
    def ls(self, dir_):
        '''
        Returns list of LsItem's.
        '''
        lines = self.telnet.ls2(dir_)
        ret = []
        for line in lines:
            if line.endswith('\r'):
                line = line[:-1]
            #print(f'line={line!r}')
            try:
                num_children, name, index, type_, value = line.split(' ', 4)
            except Exception as e:
                print(f'*** dir_={dir_!r} len(lines)={len(lines)}. failed to read items from line={line!r}. lines is: {lines!r}')
                raise
            index = int(index)
            num_children = int(num_children)
            item = LsItem(num_children, name, index, type_, value)
            #print(f'item={item}')
            ret.append( item)
        return ret

    def quit(self):
        """Close the telnet connection to FlightGear."""
        if self.telnet:
            self.telnet.quit()
            self.telnet = None

    def view_next(self):
        """Move to next view."""
        self.telnet.set( "/command/view/next", "true")

    def view_prev(self):
        """Move to next view."""
        self.telnet.set( "/command/view/prev", "true")

