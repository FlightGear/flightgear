/**************************************************************************
 * fg_zlib.h -- a zlib wrapper to replace zlib calls with normal uncompressed
 *              calls for systems that have problems building zlib.
 *
 * Written by Curtis Olson, started April 1998.
 *
 * Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
 **************************************************************************/


#ifndef _FG_ZLIB_H
#define _FG_ZLIB_H


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifdef AVOID_USING_ZLIB

  #include <stdio.h>

  #define fgFile FILE *

  /* fgFile fgopen(char *filename, const char *flags) */
  #define fgopen(P, F)  (fopen((P), (F)))

  /* int fgseek(fgFile *file, long offset, int whence) */
  #define fgseek(F, O, W)  (fseek((F), (O), (W)))

  /* fgread(fgFile file, void *buf, int size); */
  #define fgread(F, B, S)  (fread((B), (S), 1, (F)))

  /* int fggets(fgFile fd, char *buffer, int len) */
  #define fggets(F, B, L)  (fgets((B), (L), (F)))

  /* int fgclose(fgFile fd) */
  #define fgclose(F)  (fclose((F)))
#else

  #include <zlib/zlib.h>

  #define fgFile gzFile

  /* fgFile fgopen(char *filename, const char *flags) */
  #define fgopen(P, F)  (gzopen((P), (F)))

  /* int fgseek(fgFile *file, long offset, int whence) */
  #define fgseek(F, O, W)  (gzseek((F), (O), (W)))

  /* fgread(fgFile file, void *buf, int size); */
  #define fgread(F, B, S)  (gzread((F), (B), (S)))

  /* int fggets(fgFile fd, char *buffer, int len) */
  #define fggets(F, B, L)  (gzgets((F), (B), (L)))

  /* int fgclose(fgFile fd) */
  #define fgclose(F)  (gzclose((F)))

#endif /* #ifdef AVOID_USING_ZLIB #else #endif */


#endif /* _FG_ZLIB_H */


