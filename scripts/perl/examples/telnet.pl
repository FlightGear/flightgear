#!/usr/bin/perl
#
# Written by Curtis L. Olson, started December 2002
# Some code portions courtesy of Melchior FRANZ
#
# This file is in the Public Domain and comes with no warranty.
#
# $Id$
# ----------------------------------------------------------------------------


use IO::Socket;

use strict;


sub my_not() {
    my( $val ) = shift;

    if ( $val eq "true" ) {
        return 0;
    } elsif ( $val eq "false" ) {
        return 1;
    } elsif ( $val eq "" ) {
        return 1;
    } else {
        return 0;
    }
}


sub get_prop() {
    my( $handle ) = shift;

    &send( $handle, "get " . shift );
    eof $handle and die "\nconnection closed by host";
    $_ = <$handle>;
    s/\015?\012$//;
    /^-ERR (.*)/ and die "\nfgfs error: $1\n";

    return $_;
}


sub set_prop() {	
    my( $handle ) = shift;
    my( $prop ) = shift;
    my( $value ) = shift;

    &send( $handle, "set $prop $value");

    # eof $handle and die "\nconnection closed by host";
}


sub send() {
    my( $handle ) = shift;

    print $handle shift, "\015\012";
}


sub connect() {
    my( $host ) = shift;
    my( $port ) = shift;
    my( $timeout ) = (shift || 120);
    my( $socket );
    STDOUT->autoflush(1);
    while ($timeout--) {
        if ($socket = IO::Socket::INET->new( Proto => 'tcp',
                                             PeerAddr => $host,
                                             PeerPort => $port) )
        {
            $socket->autoflush(1);
            return $socket;
        }	
        print "Attempting to connect to $host ... " . $timeout . "\n";
        sleep(1);
    }
    return 0;
}


return 1;                    # make perl happy
