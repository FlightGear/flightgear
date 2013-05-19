//
// fgcom_init.hpp -- FGCOM configuration parsing and initialization
// FGCOM: Copyright (C) H. Wirtz <wirtz@dfn.de>
//
// Adaption of fg_init.h from FlightGear
// FlightGear: Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// Huge part rewritten by Tobias Ramforth to fit needs of FGCOM.
// <tobias@ramforth.com>
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <string>

enum
{
  FGCOM_OPTIONS_OK = 0,
  FGCOM_OPTIONS_HELP = 1,
  FGCOM_OPTIONS_ERROR = 2,
  FGCOM_OPTIONS_EXIT = 3,
  FGCOM_OPTIONS_VERBOSE_HELP = 4
};

/*
	option       has_param type        property         b_param s_param  func

	where:
		option    : name of the option
		has_param : option is --name=value if true or --name if false
		type      :	OPTION_BOOL    - property is a boolean
								OPTION_STRING  - property is a string
								OPTION_FLOAT   - property is a float
								OPTION_DOUBLE  - property is a double
								OPTION_FREQ    - property is a double and stands for a frequency
								OPTION_INT     - property is an integer
								OPTION_INT     - property is a char

	For OPTION_FLOAT, OPTION_DOUBLE and OPTION_INT, the parameter value is converted into a
	float, double or an integer and set to the property.
*/
enum OptionType
{
  OPTION_NONE,
  OPTION_BOOL,
  OPTION_STRING,
  OPTION_FLOAT,
  OPTION_DOUBLE,
  OPTION_FREQ,
  OPTION_INT,
  OPTION_CHAR
};

typedef struct _OptionEntry OptionEntry;

struct _OptionEntry
{
  const char *long_option;
  char option;
  bool has_param;
  enum OptionType type;
  void *parameter;
  char property;
  const char *description;
  const void *default_value;
};

// Read in configuration (file and command line)
bool fgcomInitOptions (const OptionEntry * fgcomOptions, int argc,
		       char **argv);

// fgcom usage
void fgcomUsage ();
