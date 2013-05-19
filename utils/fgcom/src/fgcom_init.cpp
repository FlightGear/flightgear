//
// fgcom_init.cpp -- FGCOM configuration parsing and initialization
// FGCOM: Copyright (C) H. Wirtz <wirtz@dfn.de>
//
// Adaption of fg_init.cxx from FlightGear
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <sstream>
#include <fstream>
//#include <stdlib.h>
//#include <string.h>             // strcmp()

#if defined( unix ) || defined( __CYGWIN__ )
#  include <unistd.h>		// for gethostname()
#endif
#if defined( _MSC_VER) || defined(__MINGW32__)
#  include <direct.h>		// for getcwd()
#  define getcwd _getcwd
#endif

#include <string>
#include <iomanip>
#include <map>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef _MSC_VER
#include "fgcom_getopt.h"
#else /* !_MSC_VER */
#include <pwd.h>
#include <getopt.h>
#endif /* _MSC_VER y/n */
#include "fgcom_init.h"
#include "fgcom.h"
#include "utils.h"

using namespace std;
using std::string;
using std::cerr;
using std::endl;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

static int io_type = -1;
static int io_level = -1;
#define SG_GENERAL  0x01
#define SG_INFO     0x01
#define SG_ALERT    0x02
#define SG_LOG(a,b,c) if ((io_type & a)&&(io_level & b)) cerr << c << endl

//
// Manipulators
//
istream& skip_eol( istream& in ) {
    char c = '\0';
    // skip to end of line.
    while ( in.get(c) ) {
        if ( (c == '\n') || (c == '\r') ) {
            break;
        }
    }
    return in;
}

istream& skip_ws( istream& in ) {
    char c;
    while ( in.get(c) ) {
        if ( ! isspace( c ) ) {
            // put back the non-space character
            in.putback(c);
            break;
        }
    }
    return in;
}

istream& skip_comment( istream& in )
{
    while ( in ) {
        // skip whitespace
        in >> skip_ws;
        char c;
        if ( in.get( c ) && c != '#' ) {
            // not a comment
            in.putback(c);
            break;
        }
        in >> skip_eol;
    }
    return in;
}


/* program name */
extern char *prog;
extern const char * default_root;

static std::string config;

static OptionEntry *fgcomOptionArray = 0;

static void _doOptions (int argc, char **argv);
static int _parseOption (const std::string & arg, const std::string & next_arg);
static void _fgcomParseArgs (int argc, char **argv);
static void _fgcomParseOptions (const std::string & path);

// Read in configuration (file and command line)
bool fgcomInitOptions (const OptionEntry * fgcomOptions, int argc, char **argv)
{
    if (!fgcomOptions) {
        std::cerr << "Error! Uninitialized fgcomOptionArray!" << std::endl;
        return false;
    }

    // set option array
    int n_options;
    for (n_options = 0; fgcomOptions[n_options].long_option != NULL; n_options++) {}

    fgcomOptionArray = (OptionEntry *) realloc (fgcomOptionArray, sizeof (OptionEntry) * (n_options + 1));
    memcpy (fgcomOptionArray, fgcomOptions, sizeof (OptionEntry) * (n_options + 1));

    // parse options
    _doOptions (argc, argv);

    return true;
}

// Create usage information out of fgcomOptionArray
void
fgcomUsage ()
{
  size_t
    max_length = 0;

  if (!fgcomOptionArray)
    {
      std::cerr <<
	"Error! Options need to be initialized by calling 'fgcomInitConfig'!"
	<< std::endl;
      return;
    }


  // find longest long_option
  const OptionEntry *
    currentEntry = &fgcomOptionArray[0];
  while (currentEntry->long_option != 0)
    {
      size_t
	current_length = strlen (currentEntry->long_option);
      if (current_length > max_length)
	{
	  max_length = current_length;
	}
      currentEntry++;
    }

  max_length *= 2;
  max_length += 10;		// for "-o, --option=, -option"

  // print head
  std::cout << "  OPTION" << std::string (max_length - 8,
					  ' ') << "\t\t" << "DESCRIPTION" <<
    std::endl;
  std::cout << std::endl;

  // iterate through option array
  currentEntry = &fgcomOptionArray[0];
  while (currentEntry->long_option != 0)
    {
      size_t
	current_length = strlen (currentEntry->long_option);

      current_length *= 2;
      current_length += 10;

      std::string option = std::string ("  -")
	+ std::string (&currentEntry->option);
      if (option.size() > 4)
          option = option.substr(0,4);
	option += std::string (", -")
	+ std::string (currentEntry->long_option)
	+ std::string (", --")
	+ std::string (currentEntry->long_option)
	+ std::string ("=") + std::string (max_length - current_length, ' ');

      std::cout << option << "\t\t" << currentEntry->description;

      if (currentEntry->has_param && (currentEntry->type != OPTION_NONE)
	  && (currentEntry->default_value != 0))
	{
	  std::cout << " (default: '";

	  if (currentEntry->type == OPTION_NONE)
	    {

	    }
	  else if (currentEntry->type == OPTION_BOOL)
	    {
	      std::cout << *(bool *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_STRING)
	    {
	      std::cout << (char *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_FLOAT)
	    {
	      std::cout << *(float *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_DOUBLE)
	    {
	      std::cout << *(double *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_FREQ)
	    {
	      std::cout << std::setw (7) << std::
		setprecision (3) << *(double *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_INT)
	    {
	      std::cout << *(int *) currentEntry->default_value;
	    }
	  else if (currentEntry->type == OPTION_CHAR)
	    {
	      std::cout << *(char *) currentEntry->default_value;
	    }

	  std::cout << "')";
	}

      std::cout << std::endl;

      currentEntry++;
    }

  std::cout << std::endl;

  std::cout << "  Available codecs:" << std::endl;
  std::cout << "  \t" <<
    "u - ulaw (default and best codec because the mixing is based onto ulaw)"
    << std::endl;
  std::cout << "  \t" << "a - alaw" << std::endl;
  std::cout << "  \t" << "g - gsm" << std::endl;
  std::cout << "  \t" << "s - speex" << std::endl;
  std::cout << "  \t" << "7 - G.723" << std::endl;

  std::cout << std::endl;
  std::cout << std::endl;

  std::cout << "  Mode 1: client for COM1 of flightgear:" << std::endl;
  std::cout << "  \t" << "$ " << prog << std::endl;
  std::cout << "  - connects " << prog << " to fgfs at localhost:" <<
    DEFAULT_FG_PORT << std::endl;
  std::cout << "  \t" << "$ " << prog << " -sother.host.tld -p23456" <<
    std::endl;
  std::cout << "  - connects " << prog << " to fgfs at other.host.tld:23456" << std::endl;

  std::cout << std::endl;

  std::cout << "  Mode 2: client for an ATC at <airport> on <frequency>:" <<
    std::endl;
  std::cout << "  \t" << "$ " << prog << " -aKSFO -f120.500" << std::endl;
  std::cout << "  - sets up " << prog <<
    " for an ATC radio at KSFO 120.500 MHz" << std::endl;

  std::cout << std::endl;
  std::cout << std::endl;

  std::cout << "Note that " << prog <<
    " starts with a guest account unless you use -U and -P!" << std::endl;

  std::cout << std::endl;
}

static char *
get_alternate_home(void)
{
	char *ah = 0;
#ifdef _MSC_VER
	char *app_data = getenv("LOCALAPPDATA");
	if (app_data) {
		ah  = _strdup(app_data);
	}
#else
      struct passwd *
	pwd = getpwent ();
      ah = strdup (pwd->pw_dir);
#endif
	return ah;
}

// Attempt to locate and parse the various non-XML config files in order
// from least precidence to greatest precidence
static void
_doOptions (int argc, char **argv)
{
  // Check for $fg_root/system.fgfsrc
  /*SGPath config( globals->get_fg_root() );
     config.append( "system.fgcomrc" );
     fgcomParseOptions(config.str()); */

  char *
    homedir = getenv ("HOME");

  if (homedir == NULL)
    {
	  homedir = get_alternate_home();
    }

/*#if defined( unix ) || defined( __CYGWIN__ )
	config.concat( "." );
	config.concat( hostname );
	fgcomParseOptions(config.str());
#endif*/

  // Check for ~/.fgfsrc
  if (homedir != NULL)
    {
      config = string (homedir);

#ifdef _WIN32
      config.append ("\\");
#else
      config.append ("/");
#endif

      config.append (".fgcomrc");
      _fgcomParseOptions (config);
    }

/*#if defined( unix ) || defined( __CYGWIN__ )
	// Check for ~/.fgfsrc.hostname
	config.concat( "." );
	config.concat( hostname );
	fgcomParseOptions(config.str());
#endif*/

  // Parse remaining command line options
  // These will override anything specified in a config file
  _fgcomParseArgs (argc, argv);
}

// lookup maps
static
  std::map <
string, string > fgcomOptionMap;
static
  std::map <
  string,
  size_t >
  fgcomLongOptionMap;

// Parse a single option
static int
_parseOption (const std::string & arg, const std::string & next_arg)
{
  if (fgcomLongOptionMap.size () == 0)
    {
      size_t
	i = 0;
      const OptionEntry *
	entry = &fgcomOptionArray[0];
      while (entry->long_option != 0)
	{
	  fgcomLongOptionMap.insert (std::pair < std::string,
				     size_t >
				     (std::string (entry->long_option), i));
	  fgcomOptionMap.insert (std::pair < std::string,
				 std::string >
				 (std::string (1, entry->option),
				  std::string (entry->long_option)));
	  i += 1;
	  entry += 1;
	}
    }

  // General Options
  if ((arg == "--help") || (arg == "-h") || (arg == "-?"))
    {
      // help/usage request
      return FGCOM_OPTIONS_HELP;
    }
  else if ((arg == "--verbose") || (arg == "-v"))
    {
      // verbose help/usage request
      return FGCOM_OPTIONS_VERBOSE_HELP;
    }
  else
    {
      std::map < string, size_t >::iterator it;
      std::string arg_name, arg_value;

      if (arg.find ("--") == 0)
	{
	  size_t
	    pos = arg.find ('=');
	  if (pos == string::npos)
	    {
	      // did not find a value
	      arg_name = arg.substr (2);

	      // now there are two possibilities:
	      //              1: this is an option without a value
	      //              2: the value can be found in next_arg
	      if (next_arg.empty ())
		{
		  // ok, value cannot be in next_arg
#ifdef DEBUG
		  std::cout << "DEBUG: option " << arg_name << std::endl;
#endif
		}
	      else
		{
		  if (next_arg.at (0) == '-')
		    {
		      // there is no value, new option starts in next_arg
#ifdef DEBUG
		      std::cout << "DEBUG: option " << arg_name << std::endl;
#endif
		    }
		  else
		    {
		      // the value is in next_arg
		      arg_value = std::string (next_arg);
#ifdef DEBUG
		      std::cout << "DEBUG: option " << arg_name << " with argument "
			<< arg_value << std::endl;
#endif
		    }
		}
	    }
	  else
	    {
	      // found a value
	      arg_name = arg.substr (2, pos - 2);
	      arg_value = arg.substr (pos + 1);

#ifdef DEBUG
	      std::cout << "DEBUG: option " << arg_name << " with argument " <<
		arg_value << std::endl;
#endif
	    }

	  it = fgcomLongOptionMap.find (arg_name);
	}
      else
	{
	  std::map < string, string >::iterator it_b;
	  arg_name = arg.substr (1, 1);
	  arg_value = arg.substr (2);

	  if (arg_name.empty ())
	    {
	      std::cerr << "Unknown option '" << arg << "'" << std::endl;
	      SG_LOG (SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'");
	      return FGCOM_OPTIONS_ERROR;
	    }

#ifdef DEBUG
	  std::cout << "DEBUG: option " << arg_name << " with argument " << arg_value
	    << std::endl;
#endif

	  it_b = fgcomOptionMap.find (arg_name);

	  if (it_b != fgcomOptionMap.end ())
	    {
	      it = fgcomLongOptionMap.find (it_b->second);
	    }
	  else
	    {
	      std::cerr << "Unknown option '" << arg << "'" << std::endl;
	      SG_LOG (SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'");
	      return FGCOM_OPTIONS_ERROR;
	    }
	}

      if (it != fgcomLongOptionMap.end ())
	{
	  const OptionEntry *
	    entry = &fgcomOptionArray[it->second];
	  switch (entry->type)
	    {
	    case OPTION_BOOL:
	      *(bool *) entry->parameter = true;
	      break;
	    case OPTION_STRING:
	      if (entry->has_param && !arg_value.empty ())
		{
		  *(char **) entry->parameter = strdup (arg_value.c_str ());
		}
	      else if (entry->has_param)
		{
		  std::cerr << "Option '" << arg << "' needs a parameter" <<
		    std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' needs a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      else
		{
		  std::cerr << "Option '" << arg <<
		    "' does not have a parameter" << std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' does not have a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      break;
	    case OPTION_FLOAT:
	      if (!arg_value.empty ())
		{
#ifdef _MSC_VER
		  float temp = atof(arg_value.c_str ());
#else // !_MSC_VER
		  char *
		    end;
		  float
		    temp = strtof (arg_value.c_str (), &end);

		  errno = 0;

		  if (*end != '\0')
		    {
		      std::cerr << "Cannot parse float value '" << arg_value
			<< "' for option " << arg_name << "!" << std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Cannot parse float value '" << arg_value <<
			      "' for option " << arg_name << "!");
		      return FGCOM_OPTIONS_ERROR;
		    }
#endif // _MSC_VER y/n

		  *(float *) (entry->parameter) = temp;
		  if (*(float *) (entry->parameter) != temp
		      || errno == ERANGE)
		    {
		      std::cerr << "Float value '" << arg_value <<
			"' for option " << arg_name << " out of range!" <<
			std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Float value '" << arg_value << "' for option "
			      << arg_name << " out of range!");
		      return FGCOM_OPTIONS_ERROR;
		    }
		}
	      else
		{
		  std::cerr << "Option '" << arg << "' needs a parameter" <<
		    std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' needs a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      break;
	    case OPTION_DOUBLE:
        case OPTION_FREQ:
	      if (!arg_value.empty ())
		{
		  char *
		    end;
		  double
		    temp = strtod (arg_value.c_str (), &end);

		  errno = 0;

		  if (*end != '\0')
		    {
		      std::cerr << "Cannot parse double value '" << arg_value
			<< "' for option " << arg_name << "!" << std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Cannot parse double value '" << arg_value <<
			      "' for option " << arg_name << "!");
		      return FGCOM_OPTIONS_ERROR;
		    }

		  *(double *) (entry->parameter) = temp;
		  if (*(double *) (entry->parameter) != temp
		      || errno == ERANGE)
		    {
		      std::cerr << "Double value '" << arg_value <<
			"' for option " << arg_name << " out of range!" <<
			std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Double value '" << arg_value << "' for option "
			      << arg_name << " out of range!");
		      return FGCOM_OPTIONS_ERROR;
		    }
		}
	      else
		{
		  std::cerr << "Option '" << arg << "' needs a parameter" <<
		    std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' needs a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      break;
	    case OPTION_INT:
	      if (!arg_value.empty ())
		{
		  char *
		    end;
		  long
		    temp = strtol (arg_value.c_str (), &end, 0);

		  errno = 0;

		  if (*end != '\0')
		    {
		      std::cerr << "Cannot parse integer value '" << arg_value
			<< "' for option " << arg_name << "!" << std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Cannot parse integer value '" << arg_value <<
			      "' for option " << arg_name << "!");
		      return FGCOM_OPTIONS_ERROR;
		    }

		  *(int *) (entry->parameter) = temp;
		  if (*(int *) (entry->parameter) != temp || errno == ERANGE)
		    {
		      std::cerr << "Integer value '" << arg_value <<
			"' for option " << arg_name << " out of range!" <<
			std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Integer value '" << arg_value <<
			      "' for option " << arg_name <<
			      " out of range!");
		      return FGCOM_OPTIONS_ERROR;
		    }
		}
	      else
		{
		  std::cerr << "Option '" << arg << "' needs a parameter" <<
		    std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' needs a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      break;
	    case OPTION_CHAR:
	      if (entry->has_param && !arg_value.empty ())
		{
		  if (arg_value.length () > 1)
		    {
		      std::cerr << "Option '" << arg <<
			"' needs a single char as parameter" << std::endl;
		      SG_LOG (SG_GENERAL, SG_ALERT,
			      "Option '" << arg <<
			      "' needs a single char as parameter");
		      return FGCOM_OPTIONS_ERROR;
		    }
		  else
		    {
		      *(char *) entry->parameter = arg_value.c_str ()[0];
		    }
		}
	      else if (entry->has_param)
		{
		  std::cerr << "Option '" << arg << "' needs a parameter" <<
		    std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' needs a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      else
		{
		  std::cerr << "Option '" << arg <<
		    "' does not have a parameter" << std::endl;
		  SG_LOG (SG_GENERAL, SG_ALERT,
			  "Option '" << arg << "' does not have a parameter");
		  return FGCOM_OPTIONS_ERROR;
		}
	      break;
	    case OPTION_NONE:
	      *(bool *) entry->parameter = true;
	      break;
	    }
	}
      else
	{
	  std::cerr << "Unknown option '" << arg << "'" << std::endl;
	  SG_LOG (SG_GENERAL, SG_ALERT, "Unknown option '" << arg << "'");
	  return FGCOM_OPTIONS_ERROR;
	}
    }

  return FGCOM_OPTIONS_OK;
}

// Parse the command line options
static void
_fgcomParseArgs (int argc, char **argv)
{
#ifdef DEBUG
  std::cout << "DEBUG: Processing commandline options." << std::endl;
#endif

  for (int i = 1; i < argc; i++)
    {
      std::string arg = std::string (argv[i]);
      std::string next_arg;
      if (i < argc - 1)
	{
	  next_arg = std::string (argv[i + 1]);
	}

      if (arg.find ('-') == 0)
	{
	  if (arg == "--")
	    {
	      // do nothing
	    }
	  else
	    {
	      int
		result = _parseOption (arg, next_arg);

	      if (result == FGCOM_OPTIONS_OK)
		{
		  // that is great
		}
	      else if (result == FGCOM_OPTIONS_HELP)
		{
		  fgcomUsage ();
		  exit (0);
		}
	      else
		{
		  std::cout << "Error parsing commandline options!" << std::
		    endl;
		  exit (1);
		}
	    }
	}
    }

  std::cout << "Successfully parsed commandline options." << std::endl;
}

// Parse config file options
static void
_fgcomParseOptions (const std::string & path)
{
    if (is_file_or_directory(path.c_str()) != 1) {
#ifdef DEBUG
        SG_LOG(SG_GENERAL, SG_ALERT, "Error: Unable to open " << path);
#endif /* ONLY FOR DEBUG */
        return;
    }

    std::fstream in;
    std::ios_base::openmode mode = std::ios_base::in;
    in.open(path.c_str(),mode);
    if (!in.is_open ()) {
#ifdef DEBUG
        SG_LOG(SG_GENERAL, SG_ALERT, "Error: DEBUG: Unable to open " << path);
#endif /* ONLY FOR DEBUG */
        return;
    }


    SG_LOG (SG_GENERAL, SG_INFO, "Processing config file: " << path);

    in >> skip_comment;
    while (!in.eof ()) {
        std::string line;
        getline (in, line, '\n');

        // catch extraneous (DOS) line ending character
        int i;
        for (i = line.length(); i > 0; i--) {
            if (line[i - 1] > 32) {
                break;
            }
        }

        line = line.substr(0, i);

        std::string next_arg;
        if (_parseOption (line, next_arg) == FGCOM_OPTIONS_ERROR) {
            SG_LOG(SG_GENERAL, SG_ALERT, "ERROR: Config file parse error: " << path << " '" << line << "'" );
            fgcomUsage ();
            exit(1);
        }

        in >> skip_comment;
    }
}

/* eof - fgcom_init.cpp */
