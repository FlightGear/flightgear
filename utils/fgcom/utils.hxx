/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__

/* Initialize the file parser. */
int parser_init(const char *filename);

/* Exits parser. */
void parser_exit(void);

int parser_get_next_value(double *value);

extern int is_file_or_directory( const char * path ); /* 1=file, 2=dir, else 0 */
extern void trim_base_path_ib( char *path ); /* trim to base path IN BUFFER */
extern int get_data_path_per_os( char *path, size_t len ); /* get data path per OS - 0=ok, 1=error */

#endif
