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

/// option processing can have various result values
/// depending on what the user requested. Note processOptions only
/// returns a subset of these.
enum OptionResult
{
    FG_OPTIONS_OK = 0,
    FG_OPTIONS_HELP = 1,
    FG_OPTIONS_ERROR = 2,
    FG_OPTIONS_EXIT = 3,
    FG_OPTIONS_VERBOSE_HELP = 4,
    FG_OPTIONS_SHOW_AIRCRAFT = 5,
    FG_OPTIONS_SHOW_SOUND_DEVICES = 6,
    FG_OPTIONS_NO_DEFAULT_CONFIG = 7
};
    
class Options
{
private:
  Options();
  
public:
  static Options* sharedInstance();

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
   * on startup - preferences.xml, fgfs.rc in various places and so on.
   * --no-default-config allows this behaviour to be changed, so only
   * expicitly listed files are read - this is useful for testing. Expose
   * the value of the option here.
   */
  bool shouldLoadDefaultConfig() const;

  /**
   * check if the arguments array contains a particular string (with a '--' or
   * '-' prefix).
   * Used by early startup code before Options object is created
   */
  static bool checkForArg(int argc, char* argv[], const char* arg);
private:
  void showUsage() const;
  
  int parseOption(const std::string& s);
  
  void processArgResult(int result);
  
  void setupRoot();
  
  std::string platformDefaultRoot() const;
  
  class OptionsPrivate;
  std::auto_ptr<OptionsPrivate> p;
};
  
} // of namespace flightgear

void fgSetDefaults();

#endif /* _OPTIONS_HXX */
