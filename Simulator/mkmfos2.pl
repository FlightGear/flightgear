#!/usr/local/bin/perl
#---------------------------------------------------------------------------
# mkmfos2 = MaKe MakeFile OS2
#
# Written by Curtis Olson, started July 1997.
#
# Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


while ( <> ) {
    s/\.o\b/\.obj/g;
    s/\.a\b/\.lib/g;
    s/\bdepend\b/depend.os2/g;
    s/\$\(MAKE\)/\$\(MAKE\) -f Makefile.os2/g;
    print $_;
}


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/07/20 02:18:32  curt
# Initial revision.
#
