// -*- coding: utf-8 -*-
//
// AddonVersion.hxx --- Version class for FlightGear add-ons
// Copyright (C) 2017  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#ifndef FG_ADDONVERSION_HXX
#define FG_ADDONVERSION_HXX

#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>

#include <simgear/nasal/cppbind/NasalCallContext.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include "addon_fwd.hxx"

namespace flightgear
{

namespace addons
{

// Order matters for the sorting/comparison functions
enum class AddonVersionSuffixPrereleaseType {
  alpha = 0,
  beta,
  candidate,
  none
};

// ***************************************************************************
// *                           AddonVersionSuffix                            *
// ***************************************************************************

class AddonVersionSuffix
{
public:
  AddonVersionSuffix(AddonVersionSuffixPrereleaseType _preReleaseType
                     = AddonVersionSuffixPrereleaseType::none,
                     int preReleaseNum = 0, bool developmental = false,
                     int devNum = 0);
  // Construct from a string. The empty string is a valid input.
  AddonVersionSuffix(const std::string& suffix);
  AddonVersionSuffix(const char* suffix);
  // Construct from a tuple
  explicit AddonVersionSuffix(
    const std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>& t);

  // Return all components of an AddonVersionSuffix instance as a tuple.
  // Beware, this is not suitable for sorting! cf. genSortKey() below.
  std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int> makeTuple() const;

  // String representation of an AddonVersionSuffix
  std::string str() const;

private:
  // String representation of the release type component: "a", "b", "rc" or "".
  static std::string releaseTypeStr(AddonVersionSuffixPrereleaseType);

  // If 's' starts with a non-empty release type ('a', 'b' or 'rc'), return
  // the corresponding enum value along with the remainder of 's' (that is,
  // everything after the release type). Otherwise, return
  // AddonVersionSuffixPrereleaseType::none along with a copy of 's'.
  static std::tuple<AddonVersionSuffixPrereleaseType, std::string>
  popPrereleaseTypeFromBeginning(const std::string& s);

  // Extract all components from a string representing a version suffix.
  // The components of the return value are, in this order:
  //
  //   preReleaseType, preReleaseNum, developmental, devNum
  //
  // Note: the empty string is a valid input.
  static std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>
  suffixStringToTuple(const std::string& suffix);

  // Used to implement suffixStringToTuple() for compilers that are not
  // C++11-compliant (gcc 4.8 pretends to support <regex> as required by C++11
  // but doesn't, see <https://stackoverflow.com/a/12665408/4756009>).
  //
  // The bool in the first component of the result is true iff 'suffix' is a
  // valid version suffix string. The bool is false when 'suffix' is invalid
  // in such a way that the generic sg_format_exception thrown at the end of
  // suffixStringToTuple() is appropriate. In all other cases, a specific
  // exception is thrown.
  static std::tuple<bool, AddonVersionSuffixPrereleaseType, int, bool, int>
  parseVersionSuffixString_noRegexp(const std::string& suffix);

  // Useful for comparisons/sorting purposes
  std::tuple<int,
             std::underlying_type<AddonVersionSuffixPrereleaseType>::type,
             int, int, int> genSortKey() const;

  friend bool operator==(const AddonVersionSuffix& lhs,
                         const AddonVersionSuffix& rhs);
  friend bool operator<(const AddonVersionSuffix& lhs,
                        const AddonVersionSuffix& rhs);

  AddonVersionSuffixPrereleaseType _preReleaseType;
  int _preReleaseNum;           // integer >= 1 (0 when not applicable)
  bool _developmental;          // whether the suffix ends with '.devN'
  int _devNum;                  // integer >= 1 (0 when not applicable)
};


// operator==() and operator<() are declared above.
bool operator!=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs);
bool operator> (const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs);
bool operator<=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs);
bool operator>=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs);

std::ostream& operator<<(std::ostream&, const AddonVersionSuffix&);

// ***************************************************************************
// *                              AddonVersion                               *
// ***************************************************************************

// I suggest to use either the year-based FlightGear-type versioning, or
// semantic versioning (<http://semver.org/>). For the suffix, we allow things
// like "a1" (alpha1), "b2" (beta2), "rc4" (release candidate 4), "a1.dev3"
// (development release for "a1", which sorts before "a1"), etc. It's a subset
// of the syntax allowed in <https://www.python.org/dev/peps/pep-0440/>.
class AddonVersion : public SGReferenced
{
public:
  AddonVersion(int major = 0, int minor = 0, int patchLevel = 0,
               AddonVersionSuffix suffix = AddonVersionSuffix());
  AddonVersion(const std::string& version);
  AddonVersion(const char* version);
  explicit AddonVersion(const std::tuple<int, int, int, AddonVersionSuffix>& t);

  // Using the method names major() and minor() can lead to incomprehensible
  // errors such as "major is not a member of flightgear::addons::AddonVersion"
  // because of a hideous glibc bug[1]: major() and minor() are defined by
  // standard headers as *macros*!
  //
  //   [1] https://bugzilla.redhat.com/show_bug.cgi?id=130601
  int majorNumber() const;
  int minorNumber() const;
  int patchLevel() const;
  AddonVersionSuffix suffix() const;
  std::string suffixStr() const;

  std::string str() const;

  // For the Nasal bindings (otherwise, we have operator==(), etc.)
  bool equal(const nasal::CallContext& ctx) const;
  bool nonEqual(const nasal::CallContext& ctx) const;
  bool lowerThan(const nasal::CallContext& ctx) const;
  bool lowerThanOrEqual(const nasal::CallContext& ctx) const;
  bool greaterThan(const nasal::CallContext& ctx) const;
  bool greaterThanOrEqual(const nasal::CallContext& ctx) const;

  static void setupGhost(nasal::Hash& addonsModule);

private:
  // Useful for comparisons/sorting purposes
  std::tuple<int, int, int, AddonVersionSuffix> makeTuple() const;

  static std::tuple<int, int, int, AddonVersionSuffix>
  versionStringToTuple(const std::string& versionStr);

  // Used to implement versionStringToTuple() for compilers that are not
  // C++11-compliant (gcc 4.8 pretends to support <regex> as required by C++11
  // but doesn't, see <https://stackoverflow.com/a/12665408/4756009>).
  //
  // The bool in the first component of the result is true iff 'versionStr' is
  // a valid version string. The bool is false when 'versionStr' is invalid in
  // such a way that the generic sg_format_exception thrown at the end of
  // versionStringToTuple() is appropriate. In all other cases, a specific
  // exception is thrown.
  static std::tuple<bool, int, int, int, AddonVersionSuffix>
  parseVersionString_noRegexp(const std::string& versionStr);

  friend bool operator==(const AddonVersion& lhs, const AddonVersion& rhs);
  friend bool operator<(const AddonVersion& lhs, const AddonVersion& rhs);

  int _major;
  int _minor;
  int _patchLevel;
  AddonVersionSuffix _suffix;
};

// operator==() and operator<() are declared above.
bool operator!=(const AddonVersion& lhs, const AddonVersion& rhs);
bool operator> (const AddonVersion& lhs, const AddonVersion& rhs);
bool operator<=(const AddonVersion& lhs, const AddonVersion& rhs);
bool operator>=(const AddonVersion& lhs, const AddonVersion& rhs);

std::ostream& operator<<(std::ostream&, const AddonVersion&);

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDONVERSION_HXX
