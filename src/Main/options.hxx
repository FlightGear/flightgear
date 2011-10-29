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
   * (set properties, etc)
   */
  void processOptions();
  
  /**
   * init the aircraft options
   */
  void initAircraft();
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

#endif /* _OPTIONS_HXX */
