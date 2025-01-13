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

#include <simgear/misc/simgear_optional.hxx>
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
  OptionResult init(int argc, char* argv[], const SGPath& appDataPath);

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
    * Check if a particular option has been set (so far).
    * For boolean option please use isBoolOptionEnable or isBoolOptionDisable.
    */
  bool isOptionSet(const std::string& key) const;
  
  /**
   * Check if the user has specified a given boolean option.
   * We need to return 3 states:
   * *  1 - the user has explicitly enabled the option,
   * *  0 - the user has explicitly disabled the option,
   * * -1 - the user has not used the specified option at all.
   *
   * User provided options =>  Using the method                  =>  Result
   * --enable-fullscreen   =>  checkBoolOptionSet("fullscreen")  =>  true
   * --disable-fullscreen  =>  checkBoolOptionSet("fullscreen")  =>  false
   * --fullscreen          =>  checkBoolOptionSet("fullscreen")  =>  true
   * --fullscreen true     =>  checkBoolOptionSet("fullscreen")  =>  true
   * --fullscreen false    =>  checkBoolOptionSet("fullscreen")  =>  false
   * --fullscreen 1        =>  checkBoolOptionSet("fullscreen")  =>  true
   * --fullscreen 0        =>  checkBoolOptionSet("fullscreen")  =>  false
   * --fullscreen yes      =>  checkBoolOptionSet("fullscreen")  =>  true
   * --fullscreen no       =>  checkBoolOptionSet("fullscreen")  =>  false
   * {none of the above}   =>  checkBoolOptionSet("fullscreen")  =>  no value
   */
  simgear::optional<bool> checkBoolOptionSet(const std::string& key) const;

  /**
   * An overlay on checkBoolOptionSet, except that when the user has not used
   * the option at all then false is returned.
   * For non-boolean option please use isOptionSet.
   */
  bool isBoolOptionEnable(const std::string &key) const;

  /**
   * An overlay on checkBoolOptionSet, to check whether user used the option
   * with explicitly disable it.
   * For non-boolean option please use isOptionSet.
   */
  bool isBoolOptionDisable(const std::string &key) const;

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
  OptionResult initAircraft();
  
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
   * Check if the arguments array contains a particular string (with a '--' or
   * '-' prefix).
   * Used by early startup code before Options object is created.
   * For boolean option please use checkForBoolArg or checkForArgEnable/checkForArgDisable.
   */
  static bool checkForArg(int argc, char* argv[], const char* arg);

  /**
   * Check if the user has specified a given boolean option.
   * Used by early startup code before Options object is created.
   * We need to return 3 states:
   * *  1 - the user has explicitly enabled the option,
   * *  0 - the user has explicitly disabled the option,
   * * -1 - the user has not used the specified option at all.
   *
   * User provided options =>  Using the method                           =>  Result
   * --enable-fullscreen   =>  checkForBoolArg(argc, argv, "fullscreen")  =>  true
   * --disable-fullscreen  =>  checkForBoolArg(argc, argv, "fullscreen")  =>  false
   * --fullscreen          =>  checkForBoolArg(argc, argv, "fullscreen")  =>  true
   * --fullscreen true     =>  checkForBoolArg(argc, argv, "fullscreen")  =>  true
   * --fullscreen false    =>  checkForBoolArg(argc, argv, "fullscreen")  =>  false
   * --fullscreen 1        =>  checkForBoolArg(argc, argv, "fullscreen")  =>  true
   * --fullscreen 0        =>  checkForBoolArg(argc, argv, "fullscreen")  =>  false
   * --fullscreen yes      =>  checkForBoolArg(argc, argv, "fullscreen")  =>  true
   * --fullscreen no       =>  checkForBoolArg(argc, argv, "fullscreen")  =>  false
   * {none of the above}   =>  checkForBoolArg(argc, argv, "fullscreen")  =>  no value
   */
  static simgear::optional<bool> checkForBoolArg(int argc, char* argv[], const std::string& checkArg);

  /**
   * Return true when user explicitly enabled boolean option, otherwise false.
   * Used by early startup code before Options object is created.
   */
  static bool checkForArgEnable(int argc, char* argv[], const std::string& checkArg);

  /**
   * Return true when user explicitly disabled boolean option by set false value.
   * Used by early startup code before Options object is created.
   */
  static bool checkForArgDisable(int argc, char* argv[], const std::string& checkArg);

  /**
   * @brief getArgValue - get the value of an argument if it exists, or
   * an empty string otherwise
   * @param argc
   * @param argv
   * @param checkArg : arg to look for, with '--' prefix
   * @return value following '=' until the next white space
   */
  static std::string getArgValue(int argc, char* argv[], const char* checkArg);


  /**
      * @brief Default location to find FGData. This is based on compile-time configuration
      * and platform conventions. For most deployments it's empty because we no longer
      * bundle FGData with the simulator, but download it automatically.
      * 
      * @return SGPath 
      */
  SGPath platformDefaultRoot() const;

  /**
      * @brief Default location to download / update FGData. In older versions this
      * was located inside the application (eg Contents/Resources on macOS). But
      * now we download the data, it needs to be user-writeable.
      *
      * The value is computed based on actualDownloadDir at present
      * 
      * @return SGPath 
      */
  SGPath downloadedDataRoot() const;

  /**
       * @brief extractOptions - extract the currently set options as
       * a string array. This can be used to examine what options were
       * requested / set so far.
       * @return
       */
  string_list extractOptions() const;

  /**
        @brief the actual download dir in use, which may be the default or a user-supplied value
     */
  SGPath actualDownloadDir() const;

  /**
       * Convert string to bool for boolean options. When param cannot be recognized as bool then
       * the true is returned.
       */
  static bool paramToBool(const std::string& param);

  private:
      void showUsage() const;
      void showVersion() const;
      // Write info such as FG version, FG_ROOT, FG_HOME, scenery paths, aircraft
      // paths, etc. to stdout in JSON format, using the UTF-8 encoding.
      void printJSONReport() const;

      // The 'fromConfigFile' parameter indicates whether the option comes from a
      // config file or directly from the command line.
      int parseOption(const std::string& s, const simgear::optional<std::string>& val, bool fromConfigFile);

      int parseConfigOption(const SGPath &path, bool fromConfigFile);

      std::string getValueForBooleanOption(const std::string& str, const std::string& option, const simgear::optional<std::string>& value);

      /**
       * Since option values can be separated by a space, we check what is in
       * the next parameter and return a string value if the current option
       * requires a value and the value does not start with a "-" character.
       */
      static simgear::optional<std::string> getValueFromNextParam(int index, int argc, char** argv);

      void processArgResult(int result);

      /**
     * Setup the root base, and check it's valid. If
     * the root package was not found or is the incorrect version,
     * returns FG_OPTIONS_ERROR. Argv/argv
     * are passed since we might potentially show a GUI dialog at this point
     * to help the user our (finding a base package), and hence need to init Qt.
     */
      OptionResult setupRoot(int argc, char** argv);


      class OptionsPrivate;
      std::unique_ptr<OptionsPrivate> p;
};
  
} // of namespace flightgear

void fgSetDefaults();

#endif /* _OPTIONS_HXX */
