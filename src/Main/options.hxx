// options.hxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$


#ifndef _OPTIONS_HXX
#define _OPTIONS_HXX

#include <memory>
#include <string>

#include <simgear/misc/strutils.hxx>

// forward decls
class SGPath;

namespace flightgear
{

    /**
     * return the default platform dependant download directory.
     * This must be a user-writeable location, the question is if it should
     * be a user visible location. On Windows we default to a subdir of
     * Documents (FlightGear), on Unixes we default to FG_HOME, which is
     * typically invisible.
     */
    SGPath defaultDownloadDir();

/// option processing can have various result values
/// depending on what the user requested. Note processOptions only
/// returns a subset of these.
enum OptionResult
{
    FG_OPTIONS_OK = 0,
    FG_OPTIONS_HELP,
    FG_OPTIONS_ERROR,
    FG_OPTIONS_EXIT,
    FG_OPTIONS_VERBOSE_HELP,
    FG_OPTIONS_SHOW_AIRCRAFT,
    FG_OPTIONS_SHOW_SOUND_DEVICES,
    FG_OPTIONS_NO_DEFAULT_CONFIG
};
    
class Options
{
private:
  Options();
  
public:
  static Options* sharedInstance();

    /**
     * Delete the entire options object. Use with a degree of care, no code
     * should ever be caching the Options pointer but this has not actually been
     * checked across the whole code :)
     */
    static void reset();

  ~Options();
  
  /**
   * pass command line arguments, read default config files
   */
  void init(int argc, char* argv[], const SGPath& appDataPath);
  
  /**
    * parse a config file (eg, .fgfsrc) 
    */
  void readConfig(const SGPath& path);
  
  /**
    * read the value for an option, if it has been set
    */
  std::string valueForOption(const std::string& key, const std::string& defValue = std::string()) const;
  
  /**
    * return all values for a multi-valued option
    */
  string_list valuesForOption(const std::string& key) const;
  
  /**
    * check if a particular option has been set (so far)
    */
  bool isOptionSet(const std::string& key) const;
  
  
  /**
    * set an option value, assuming it is not already set (or multiple values
    * are permitted)
    * This can be used to inject option values, eg based upon environment variables
    */
  int addOption(const std::string& key, const std::string& value);

  /**
   * set an option, overwriting any existing value which might be set
   */
  int setOption(const std::string& key, const std::string& value);

  void clearOption(const std::string& key);

  /**
   * apply option values to the simulation state
   * (set properties, etc). 
   */
  OptionResult processOptions();

    /**
     * process command line options relating to scenery / aircraft / data paths
     */
    void initPaths();

  /**
   * init the aircraft options
   */
  void initAircraft();
  
  /**
   * should defualt configuration files be loaded and processed or not?
   * There's many configuration files we have historically read by default
   * on startup - fgfs.rc in various places and so on.
   * --no-default-config allows this behaviour to be changed, so only
   * expicitly listed files are read Expose
   * the value of the option here.
   */
  bool shouldLoadDefaultConfig() const;

    /**
     * when using the built-in launcher, we disable the default config files.
     * explicitly loaded confg files are still permitted.
     */
    void setShouldLoadDefaultConfig(bool load);

  /**
   * check if the arguments array contains a particular string (with a '--' or
   * '-' prefix).
   * Used by early startup code before Options object is created
   */
  static bool checkForArg(int argc, char* argv[], const char* arg);

      SGPath platformDefaultRoot() const;
private:
  void showUsage() const;
  void showVersion() const;
  // Write info such as FG version, FG_ROOT, FG_HOME, scenery paths, aircraft
  // paths, etc. to stdout in JSON format, using the UTF-8 encoding.
  void printJSONReport() const;

  int parseOption(const std::string& s);
  
  void processArgResult(int result);

    /**
     * Setup the root base, and check it's valid. Bails out with exit(-1) if
     * the root package was not found or is the incorrect version. Argv/argv
     * are passed since we might potentially show a GUI dialog at this point
     * to help the user our (finding a base package), and hence need to init Qt.
     */
  void setupRoot(int argc, char **argv);

  
  class OptionsPrivate;
  std::unique_ptr<OptionsPrivate> p;
};
  
} // of namespace flightgear

void fgSetDefaults();

#endif /* _OPTIONS_HXX */
