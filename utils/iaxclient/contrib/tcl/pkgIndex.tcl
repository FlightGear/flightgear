#
# Tcl package index file
#
package ifneeded iaxclient 0.2 \
    "[list load [file join $dir iaxclient.dylib] iaxclient]; \
    [list source [file join $dir iaxclient.tcl]]"

