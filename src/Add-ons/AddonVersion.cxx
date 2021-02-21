// -*- coding: utf-8 -*-
//
// AddonVersion.cxx --- Version class for FlightGear add-ons
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

#include <numeric>              // std::accumulate()
#include <ostream>
#include <regex>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>

#include <simgear/misc/strutils.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalCallContext.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

#include "addon_fwd.hxx"
#include "AddonVersion.hxx"

using std::string;
using std::vector;
using simgear::enumValue;

namespace strutils = simgear::strutils;

namespace flightgear
{

namespace addons
{

// ***************************************************************************
// *                           AddonVersionSuffix                            *
// ***************************************************************************

AddonVersionSuffix::AddonVersionSuffix(
  AddonVersionSuffixPrereleaseType preReleaseType, int preReleaseNum,
  bool developmental, int devNum)
  : _preReleaseType(preReleaseType),
    _preReleaseNum(preReleaseNum),
    _developmental(developmental),
    _devNum(devNum)
{ }

// Construct an AddonVersionSuffix instance from a tuple (preReleaseType,
// preReleaseNum, developmental, devNum). This would be nicer with
// std::apply(), but it requires C++17.
AddonVersionSuffix::AddonVersionSuffix(
  const std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>& t)
  : AddonVersionSuffix(std::get<0>(t), std::get<1>(t), std::get<2>(t),
                       std::get<3>(t))
{ }

AddonVersionSuffix::AddonVersionSuffix(const std::string& suffix)
  : AddonVersionSuffix(suffixStringToTuple(suffix))
{ }

AddonVersionSuffix::AddonVersionSuffix(const char* suffix)
  : AddonVersionSuffix(string(suffix))
{ }

// Static method
string
AddonVersionSuffix::releaseTypeStr(AddonVersionSuffixPrereleaseType releaseType)
{
  switch (releaseType) {
  case AddonVersionSuffixPrereleaseType::alpha:
    return string("a");
  case AddonVersionSuffixPrereleaseType::beta:
    return string("b");
  case AddonVersionSuffixPrereleaseType::candidate:
    return string("rc");
  case AddonVersionSuffixPrereleaseType::none:
    return string();
  default:
    throw sg_error("unexpected value for member of "
                   "flightgear::addons::AddonVersionSuffixPrereleaseType: " +
                   std::to_string(enumValue(releaseType)));
  }
}

string
AddonVersionSuffix::str() const
{
  string res = releaseTypeStr(_preReleaseType);

  if (!res.empty()) {
    res += std::to_string(_preReleaseNum);
  }

  if (_developmental) {
    res += ".dev" + std::to_string(_devNum);
  }

  return res;
}

// Static method
std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>
AddonVersionSuffix::suffixStringToTuple(const std::string& suffix)
{
#ifdef HAVE_WORKING_STD_REGEX
  // Use a simplified variant of the syntax described in PEP 440
  // <https://www.python.org/dev/peps/pep-0440/>: for the version suffix, only
  // allow a pre-release segment and a development release segment, but no
  // post-release segment.
  std::regex versionSuffixRegexp(R"((?:(a|b|rc)(\d+))?(?:\.dev(\d+))?)");
  std::smatch results;

  if (std::regex_match(suffix, results, versionSuffixRegexp)) {
    const string preReleaseType_s = results.str(1);
    const string preReleaseNum_s = results.str(2);
    const string devNum_s = results.str(3);

    AddonVersionSuffixPrereleaseType preReleaseType = AddonVersionSuffixPrereleaseType::none;
    int preReleaseNum = 0;
    int devNum = 0;

    if (preReleaseType_s.empty()) {
      preReleaseType = AddonVersionSuffixPrereleaseType::none;
    } else {
      if (preReleaseType_s == "a") {
        preReleaseType = AddonVersionSuffixPrereleaseType::alpha;
      } else if (preReleaseType_s == "b") {
        preReleaseType = AddonVersionSuffixPrereleaseType::beta;
      } else if (preReleaseType_s == "rc") {
        preReleaseType = AddonVersionSuffixPrereleaseType::candidate;
      } else {
        assert(false);          // the regexp should prevent this
      }

      assert(!preReleaseNum_s.empty());
      preReleaseNum = strutils::readNonNegativeInt<int>(preReleaseNum_s);

      if (preReleaseNum < 1) {
        string msg = "invalid add-on version suffix: '" + suffix + "' "
          "(prerelease number must be greater than or equal to 1, but got " +
          preReleaseNum_s + ")";
        throw sg_format_exception(msg, suffix);
      }
    }

    if (!devNum_s.empty()) {
      devNum = strutils::readNonNegativeInt<int>(devNum_s);

      if (devNum < 1) {
        string msg = "invalid add-on version suffix: '" + suffix + "' "
          "(development release number must be greater than or equal to 1, "
          "but got " + devNum_s + ")";
        throw sg_format_exception(msg, suffix);
      }
    }

    return std::make_tuple(preReleaseType, preReleaseNum, !devNum_s.empty(),
                           devNum);
#else // all this 'else' clause should be removed once we actually require C++11
  bool isMatch;
  AddonVersionSuffixPrereleaseType preReleaseType;
  int preReleaseNum;
  bool developmental;
  int devNum;

  std::tie(isMatch, preReleaseType, preReleaseNum, developmental, devNum) =
    parseVersionSuffixString_noRegexp(suffix);

  if (isMatch) {
    return std::make_tuple(preReleaseType, preReleaseNum, developmental,
                           devNum);
#endif     // HAVE_WORKING_STD_REGEX
  } else {                      // the regexp didn't match
    string msg = "invalid add-on version suffix: '" + suffix + "' "
      "(expected form is [{a|b|rc}N1][.devN2] where N1 and N2 are positive "
      "integers)";
    throw sg_format_exception(msg, suffix);
  }
}

// Static method, only needed for compilers that are not C++11-compliant
// (gcc 4.8 pretends to support <regex> as required by C++11 but doesn't, see
// <https://stackoverflow.com/a/12665408/4756009>).
std::tuple<bool, AddonVersionSuffixPrereleaseType, int, bool, int>
AddonVersionSuffix::parseVersionSuffixString_noRegexp(const string& suffix)
{
  AddonVersionSuffixPrereleaseType preReleaseType;
  string rest;
  int preReleaseNum = 0;        // alpha, beta or release candidate number, or
                                // 0 when absent
  bool developmental = false;   // whether 'suffix' has a .devN2 part
  int devNum = 0;               // the N2 in question, or 0 when absent

  std::tie(preReleaseType, rest) = popPrereleaseTypeFromBeginning(suffix);

  if (preReleaseType != AddonVersionSuffixPrereleaseType::none) {
    std::size_t startPrerelNum = rest.find_first_of("0123456789");
    if (startPrerelNum != 0) {    // no prerelease num -> no match
      return std::make_tuple(false, preReleaseType, preReleaseNum, false,
                             devNum);
    }

    std::size_t endPrerelNum = rest.find_first_not_of("0123456789", 1);
    // Works whether endPrerelNum is string::npos or not
    string preReleaseNum_s = rest.substr(0, endPrerelNum);
    preReleaseNum = strutils::readNonNegativeInt<int>(preReleaseNum_s);

    if (preReleaseNum < 1) {
      string msg = "invalid add-on version suffix: '" + suffix + "' "
        "(prerelease number must be greater than or equal to 1, but got " +
        preReleaseNum_s + ")";
      throw sg_format_exception(msg, suffix);
    }

    rest = (endPrerelNum == string::npos) ? "" : rest.substr(endPrerelNum);
  }

  if (strutils::starts_with(rest, ".dev")) {
    rest = rest.substr(4);
    std::size_t startDevNum = rest.find_first_of("0123456789");
    if (startDevNum != 0) {    // no dev num -> no match
      return std::make_tuple(false, preReleaseType, preReleaseNum, false,
                             devNum);
    }

    std::size_t endDevNum = rest.find_first_not_of("0123456789", 1);
    if (endDevNum != string::npos) {
      // There is trailing garbage after the development release number
      // -> no match
      return std::make_tuple(false, preReleaseType, preReleaseNum, false,
                             devNum);
    }

    devNum = strutils::readNonNegativeInt<int>(rest);
    if (devNum < 1) {
      string msg = "invalid add-on version suffix: '" + suffix + "' "
        "(development release number must be greater than or equal to 1, "
        "but got " + rest + ")";
      throw sg_format_exception(msg, suffix);
    }

    developmental = true;
  }

  return std::make_tuple(true, preReleaseType, preReleaseNum, developmental,
                         devNum);
}

// Static method
std::tuple<AddonVersionSuffixPrereleaseType, string>
AddonVersionSuffix::popPrereleaseTypeFromBeginning(const string& s)
{
  if (s.empty()) {
    return std::make_tuple(AddonVersionSuffixPrereleaseType::none, s);
  } else if (s[0] == 'a') {
    return std::make_tuple(AddonVersionSuffixPrereleaseType::alpha,
                           s.substr(1));
  } else if (s[0] == 'b') {
    return std::make_tuple(AddonVersionSuffixPrereleaseType::beta, s.substr(1));
  } else if (strutils::starts_with(s, "rc")) {
    return std::make_tuple(AddonVersionSuffixPrereleaseType::candidate,
                           s.substr(2));
  }

  return std::make_tuple(AddonVersionSuffixPrereleaseType::none, s);
}

// Beware, this is not suitable for sorting! cf. genSortKey() below.
std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>
AddonVersionSuffix::makeTuple() const
{
  return std::make_tuple(_preReleaseType, _preReleaseNum, _developmental,
                         _devNum);
}

std::tuple<int,
           std::underlying_type<AddonVersionSuffixPrereleaseType>::type,
           int, int, int>
AddonVersionSuffix::genSortKey() const
{
  using AddonRelType = AddonVersionSuffixPrereleaseType;

  // The first element means that a plain .devN is lower than everything else,
  // except .devM with M <= N (namely: all dev and non-dev alpha, beta,
  // candidates, as well as the empty suffix).
  return std::make_tuple(
    ((_developmental && _preReleaseType == AddonRelType::none) ? 0 : 1),
    enumValue(_preReleaseType),
    _preReleaseNum,
    (_developmental ? 0 : 1), // e.g., 1.0.3a2.devN < 1.0.3a2 for all N
    _devNum);
}

bool operator==(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return lhs.genSortKey() == rhs.genSortKey(); }

bool operator!=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return !operator==(lhs, rhs); }

bool operator< (const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return lhs.genSortKey() < rhs.genSortKey(); }

bool operator> (const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return operator<(rhs, lhs); }

bool operator<=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return !operator>(lhs, rhs); }

bool operator>=(const AddonVersionSuffix& lhs, const AddonVersionSuffix& rhs)
{ return !operator<(lhs, rhs); }

std::ostream& operator<<(std::ostream& os,
                         const AddonVersionSuffix& addonVersionSuffix)
{ return os << addonVersionSuffix.str(); }

// ***************************************************************************
// *                              AddonVersion                               *
// ***************************************************************************

AddonVersion::AddonVersion(int major, int minor, int patchLevel,
                           AddonVersionSuffix suffix)
    : _major(major),
      _minor(minor),
      _patchLevel(patchLevel),
      _suffix(std::move(suffix))
  { }

// Construct an AddonVersion instance from a tuple (major, minor, patchLevel,
// suffix). This would be nicer with std::apply(), but it requires C++17.
AddonVersion::AddonVersion(
  const std::tuple<int, int, int, AddonVersionSuffix>& t)
  : AddonVersion(std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t))
{ }

AddonVersion::AddonVersion(const std::string& versionStr)
  : AddonVersion(versionStringToTuple(versionStr))
{ }

AddonVersion::AddonVersion(const char* versionStr)
  : AddonVersion(string(versionStr))
{ }

// Static method
std::tuple<int, int, int, AddonVersionSuffix>
AddonVersion::versionStringToTuple(const std::string& versionStr)
{
#ifdef HAVE_WORKING_STD_REGEX
  // Use a simplified variant of the syntax described in PEP 440
  // <https://www.python.org/dev/peps/pep-0440/> (always 3 components in the
  // release segment, pre-release segment + development release segment; no
  // post-release segment allowed).
  std::regex versionRegexp(R"((\d+)\.(\d+).(\d+)(.*))");
  std::smatch results;

  if (std::regex_match(versionStr, results, versionRegexp)) {
    const string majorNumber_s = results.str(1);
    const string minorNumber_s = results.str(2);
    const string patchLevel_s = results.str(3);
    const string suffix_s = results.str(4);

    int major = strutils::readNonNegativeInt<int>(majorNumber_s);
    int minor = strutils::readNonNegativeInt<int>(minorNumber_s);
    int patchLevel = strutils::readNonNegativeInt<int>(patchLevel_s);

    return std::make_tuple(major, minor, patchLevel,
                           AddonVersionSuffix(suffix_s));
#else // all this 'else' clause should be removed once we actually require C++11
  bool isMatch;
  int major, minor, patchLevel;
  AddonVersionSuffix suffix;

  std::tie(isMatch, major, minor, patchLevel, suffix) =
    parseVersionString_noRegexp(versionStr);

  if (isMatch) {
    return std::make_tuple(major, minor, patchLevel, suffix);
#endif     // HAVE_WORKING_STD_REGEX
  } else {                      // the regexp didn't match
    string msg = "invalid add-on version number: '" + versionStr + "' "
      "(expected form is MAJOR.MINOR.PATCHLEVEL[{a|b|rc}N1][.devN2] where "
      "N1 and N2 are positive integers)";
    throw sg_format_exception(msg, versionStr);
  }
}

// Static method, only needed for compilers that are not C++11-compliant
// (gcc 4.8 pretends to support <regex> as required by C++11 but doesn't, see
// <https://stackoverflow.com/a/12665408/4756009>).
std::tuple<bool, int, int, int, AddonVersionSuffix>
AddonVersion::parseVersionString_noRegexp(const string& versionStr)
{
  int major = 0, minor = 0, patchLevel = 0;
  AddonVersionSuffix suffix{};

  // Major version number
  std::size_t endMajor = versionStr.find_first_not_of("0123456789");
  if (endMajor == 0 || endMajor == string::npos) { // no match
    return std::make_tuple(false, major, minor, patchLevel, suffix);
  }
  major = strutils::readNonNegativeInt<int>(versionStr.substr(0, endMajor));

  // Dot separating the major and minor version numbers
  if (versionStr.size() < endMajor + 1 || versionStr[endMajor] != '.') {
    return std::make_tuple(false, major, minor, patchLevel, suffix);
  }
  string rest = versionStr.substr(endMajor + 1);

  // Minor version number
  std::size_t endMinor = rest.find_first_not_of("0123456789");
  if (endMinor == 0 || endMinor == string::npos) { // no match
    return std::make_tuple(false, major, minor, patchLevel, suffix);
  }
  minor = strutils::readNonNegativeInt<int>(rest.substr(0, endMinor));

  // Dot separating the minor version number and the patch level
  if (rest.size() < endMinor + 1 || rest[endMinor] != '.') {
    return std::make_tuple(false, major, minor, patchLevel, suffix);
  }
  rest = rest.substr(endMinor + 1);

  // Patch level
  std::size_t endPatchLevel = rest.find_first_not_of("0123456789");
  if (endPatchLevel == 0) {     // no patch level, therefore no match
    return std::make_tuple(false, major, minor, patchLevel, suffix);
  }
  patchLevel = strutils::readNonNegativeInt<int>(rest.substr(0, endPatchLevel));

  if (endPatchLevel != string::npos) { // there is a version suffix, parse it
    suffix = AddonVersionSuffix(rest.substr(endPatchLevel));
  }

  return std::make_tuple(true, major, minor, patchLevel, suffix);
}

int AddonVersion::majorNumber() const
{ return _major; }

int AddonVersion::minorNumber() const
{ return _minor; }

int AddonVersion::patchLevel() const
{ return _patchLevel; }

AddonVersionSuffix AddonVersion::suffix() const
{ return _suffix; }

std::string AddonVersion::suffixStr() const
{ return suffix().str(); }

std::tuple<int, int, int, AddonVersionSuffix> AddonVersion::makeTuple() const
{
  return std::make_tuple(majorNumber(), minorNumber(), patchLevel(), suffix());
}

string AddonVersion::str() const
{
  // Assemble the major.minor.patchLevel string
  vector<int> v({majorNumber(), minorNumber(), patchLevel()});
  string relSeg = std::accumulate(std::next(v.begin()), v.end(),
                                  std::to_string(v[0]),
                                  [](string s, int num) {
                                    return s + '.' + std::to_string(num);
                                  });

  // Concatenate with the suffix string
  return relSeg + suffixStr();
}


bool operator==(const AddonVersion& lhs, const AddonVersion& rhs)
{ return lhs.makeTuple() == rhs.makeTuple(); }

bool operator!=(const AddonVersion& lhs, const AddonVersion& rhs)
{ return !operator==(lhs, rhs); }

bool operator< (const AddonVersion& lhs, const AddonVersion& rhs)
{ return lhs.makeTuple() < rhs.makeTuple(); }

bool operator> (const AddonVersion& lhs, const AddonVersion& rhs)
{ return operator<(rhs, lhs); }

bool operator<=(const AddonVersion& lhs, const AddonVersion& rhs)
{ return !operator>(lhs, rhs); }

bool operator>=(const AddonVersion& lhs, const AddonVersion& rhs)
{ return !operator<(lhs, rhs); }

std::ostream& operator<<(std::ostream& os, const AddonVersion& addonVersion)
{ return os << addonVersion.str(); }


// ***************************************************************************
// *                         For the Nasal bindings                          *
// ***************************************************************************

bool AddonVersion::equal(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this == *other;
}

bool AddonVersion::nonEqual(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this != *other;
}

bool AddonVersion::lowerThan(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this < *other;
}

bool AddonVersion::lowerThanOrEqual(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this <= *other;
}

bool AddonVersion::greaterThan(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this > *other;
}

bool AddonVersion::greaterThanOrEqual(const nasal::CallContext& ctx) const
{
  auto other = ctx.requireArg<AddonVersionRef>(0);
  return *this >= *other;
}

// Static method
void AddonVersion::setupGhost(nasal::Hash& addonsModule)
{
  nasal::Ghost<AddonVersionRef>::init("addons.AddonVersion")
    .member("majorNumber", &AddonVersion::majorNumber)
    .member("minorNumber", &AddonVersion::minorNumber)
    .member("patchLevel", &AddonVersion::patchLevel)
    .member("suffix", &AddonVersion::suffixStr)
    .method("str", &AddonVersion::str)
    .method("equal", &AddonVersion::equal)
    .method("nonEqual", &AddonVersion::nonEqual)
    .method("lowerThan", &AddonVersion::lowerThan)
    .method("lowerThanOrEqual", &AddonVersion::lowerThanOrEqual)
    .method("greaterThan", &AddonVersion::greaterThan)
    .method("greaterThanOrEqual", &AddonVersion::greaterThanOrEqual);
}

} // of namespace addons

} // of namespace flightgear
