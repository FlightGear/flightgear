/*
 * sunpos.h
 * kirk johnson
 * july 1993
 *
 * code for calculating the position on the earth's surface for which
 * the sun is directly overhead (adapted from _practical astronomy
 * with your calculator, third edition_, peter duffett-smith,
 * cambridge university press, 1988.)
 *
 * RCS $Id$
 *
 * Copyright (C) 1989, 1990, 1993, 1994, 1995 Kirk Lauritz Johnson
 *
 * Parts of the source code (as marked) are:
 *   Copyright (C) 1989, 1990, 1991 by Jim Frost
 *   Copyright (C) 1992 by Jamie Zawinski <jwz@lucid.com>
 *
 * Permission to use, copy, modify and freely distribute xearth for
 * non-commercial and not-for-profit purposes is hereby granted
 * without fee, provided that both the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.
 *
 * The author makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef _SUNPOS_H
#define _SUNPOS_H


#include <time.h>

#include <Include/types.h>

/* update the cur_time_params structure with the current sun position */
void fgUpdateSunPos( void );

void fgSunPosition(time_t ssue, double *lon, double *lat);


#endif /* _SUNPOS_H */
