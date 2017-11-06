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
                   "flightgear::AddonVersionSuffixPrereleaseType: " +
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

    AddonVersionSuffixPrereleaseType preReleaseType;
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

    return {preReleaseType, preReleaseNum, !devNum_s.empty(), devNum};
  } else {                      // the regexp didn't match
    string msg = "invalid add-on version suffix: '" + suffix + "' "
      "(expected form is [{a|b|rc}N1][.devN2] where N1 and N2 are positive "
      "integers)";
    throw sg_format_exception(msg, suffix);
  }
}

// Beware, this is not suitable for sorting! cf. genSortKey() below.
std::tuple<AddonVersionSuffixPrereleaseType, int, bool, int>
AddonVersionSuffix::makeTuple() const
{
  return { _preReleaseType, _preReleaseNum, _developmental, _devNum };
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
  return { ((_developmental && _preReleaseType == AddonRelType::none) ? 0 : 1),
           enumValue(_preReleaseType),
           _preReleaseNum,
           (_developmental ? 0 : 1), // e.g., 1.0.3a2.devN < 1.0.3a2 for all N
           _devNum };
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
  // Use a simplified variant of the syntax described in PEP 440
  // <https://www.python.org/dev/peps/pep-0440/> (always 3 components in the
  // release segment, pre-release segment + development release segment; no
  // post-release segment allowed).
  std::regex versionRegexp(
    R"((\d+)\.(\d+).(\d+)(.*))");
  std::smatch results;

  if (std::regex_match(versionStr, results, versionRegexp)) {
    const string majorNumber_s = results.str(1);
    const string minorNumber_s = results.str(2);
    const string patchLevel_s = results.str(3);
    const string suffix_s = results.str(4);

    int major = strutils::readNonNegativeInt<int>(majorNumber_s);
    int minor = strutils::readNonNegativeInt<int>(minorNumber_s);
    int patchLevel = strutils::readNonNegativeInt<int>(patchLevel_s);

    return {major, minor, patchLevel, AddonVersionSuffix(suffix_s)};
  } else {                      // the regexp didn't match
    string msg = "invalid add-on version number: '" + versionStr + "' "
      "(expected form is MAJOR.MINOR.PATCHLEVEL[{a|b|rc}N1][.devN2] where "
      "N1 and N2 are positive integers)";
    throw sg_format_exception(msg, versionStr);
  }
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
{ return {majorNumber(), minorNumber(), patchLevel(), suffix()}; }

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

} // of namespace flightgear
