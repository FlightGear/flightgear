/**************************************************************************
 * panel.hxx -- instrument panel defines and prototypes
 *
 * Written by Friedemann Reinhard, started June 1998.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/
 

#ifndef _PANEL_HXX
#define _PANEL_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


typedef struct {
    unsigned short imagic;
    unsigned short type;
    unsigned short dim;
    unsigned short sizeX, sizeY, sizeZ;
    char name[128];
    unsigned char *data;
} IMAGE;


void fgPanelInit(void);
void fgPanelUpdate(void);


#endif /* _PANEL_HXX */


/* $Log$
/* Revision 1.1  1998/06/27 16:47:55  curt
/* Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
/* first pass at an isntrument panel.
/*
 */
