// -*- coding: utf-8 -*-
//
// test_AddonManagement.cxx --- Automated tests for FlightGear classes dealing
//                              with add-ons
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

#include "test_AddonManagement.hxx"

#include "config.h"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <cassert>
#include <cstddef>

#include "Add-ons/Addon.hxx"
#include "Add-ons/AddonVersion.hxx"


using std::string;
using std::vector;

using flightgear::addons::Addon;
using flightgear::addons::AddonVersion;
using flightgear::addons::AddonVersionSuffix;



void AddonManagementTests::testAddonVersionSuffix()
{
  using AddonRelType = flightgear::addons::AddonVersionSuffixPrereleaseType;

  FGTestApi::setUp::initTestGlobals("AddonVersionSuffix");

  AddonVersionSuffix v1(AddonRelType::beta, 2, true, 5);
  AddonVersionSuffix v1Copy(v1);
  AddonVersionSuffix v1NonDev(AddonRelType::beta, 2, false);
  CPPUNIT_ASSERT_EQUAL(v1, v1Copy);
  CPPUNIT_ASSERT_EQUAL(v1, AddonVersionSuffix("b2.dev5"));
  CPPUNIT_ASSERT(v1.makeTuple() ==
                          std::make_tuple(AddonRelType::beta, 2, true, 5));
  CPPUNIT_ASSERT_EQUAL(AddonVersionSuffix(), AddonVersionSuffix(""));
  // A simple comparison
  CPPUNIT_ASSERT(v1 < v1NonDev);    // b2.dev5 < b2

  // Check string representation with str()
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none).str() == "");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none, 0, true, 12).str() ==
                 ".dev12");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::alpha, 1).str() == "a1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::alpha, 1, false).str() == "a1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::alpha, 2, true, 4).str() ==
                 "a2.dev4");

  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::beta, 1).str() == "b1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::beta, 1, false).str() == "b1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::beta, 2, true, 4).str() ==
                 "b2.dev4");

  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::candidate, 1).str() == "rc1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::candidate, 1, false).str() ==
                 "rc1");
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::candidate, 2, true, 4).str() ==
                 "rc2.dev4");

  // Check stream output
  std::ostringstream oss;
  oss << AddonVersionSuffix(AddonRelType::candidate, 2, true, 4);
  CPPUNIT_ASSERT(oss.str() == "rc2.dev4");

  // Check ordering with all types of transitions, using operator<()
  auto checkStrictOrdering = [](const vector<AddonVersionSuffix>& v) {
    assert(v.size() > 1);
    for (std::size_t i=0; i < v.size() - 1; i++) {
      CPPUNIT_ASSERT(v[i] < v[i+1]);
    }
  };

  checkStrictOrdering({
      {AddonRelType::none, 0, true, 1},
      {AddonRelType::none, 0, true, 2},
      {AddonRelType::alpha, 1, true, 1},
      {AddonRelType::alpha, 1, true, 2},
      {AddonRelType::alpha, 1, true, 3},
      {AddonRelType::alpha, 1, false},
      {AddonRelType::alpha, 2, true, 1},
      {AddonRelType::alpha, 2, true, 3},
      {AddonRelType::alpha, 2, false},
      {AddonRelType::beta, 1, true, 1},
      {AddonRelType::beta, 1, true, 25},
      {AddonRelType::beta, 1, false},
      {AddonRelType::beta, 2, true, 1},
      {AddonRelType::beta, 2, true, 2},
      {AddonRelType::beta, 2, false},
      {AddonRelType::candidate, 1, true, 1},
      {AddonRelType::candidate, 1, true, 2},
      {AddonRelType::candidate, 1, false},
      {AddonRelType::candidate, 2, true, 1},
      {AddonRelType::candidate, 2, true, 2},
      {AddonRelType::candidate, 2, false},
      {AddonRelType::candidate, 21, false},
      {AddonRelType::none}
    });

  // Check operator>() and operator!=()
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none) >
              AddonVersionSuffix(AddonRelType::candidate, 21, false));

  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none) !=
              AddonVersionSuffix(AddonRelType::candidate, 21, false));

  // Check operator<=() and operator>=()
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::candidate, 2, false) <=
              AddonVersionSuffix(AddonRelType::candidate, 2, false));
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::candidate, 2, false) <=
              AddonVersionSuffix(AddonRelType::none));

  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none) >=
              AddonVersionSuffix(AddonRelType::none));
  CPPUNIT_ASSERT(AddonVersionSuffix(AddonRelType::none) >=
              AddonVersionSuffix(AddonRelType::candidate, 21, false));

  FGTestApi::tearDown::shutdownTestGlobals();
}

void AddonManagementTests::testAddonVersion()
{
  using AddonRelType = flightgear::addons::AddonVersionSuffixPrereleaseType;

  FGTestApi::setUp::initTestGlobals("AddonVersion");

  AddonVersionSuffix suffix(AddonRelType::beta, 2, true, 5);
  AddonVersion v1(2017, 4, 7, suffix);
  AddonVersion v1Copy(v1);
  AddonVersion v2 = v1;
  AddonVersion v3(std::move(v1Copy));
  AddonVersion v4 = std::move(v2);

  CPPUNIT_ASSERT_EQUAL(v1, AddonVersion("2017.4.7b2.dev5"));
  CPPUNIT_ASSERT_EQUAL(v1, AddonVersion(std::make_tuple(2017, 4, 7, suffix)));
  CPPUNIT_ASSERT_EQUAL(v1, v3);
  CPPUNIT_ASSERT_EQUAL(v1, v4);
  CPPUNIT_ASSERT(v1 < AddonVersion("2017.4.7b2"));
  CPPUNIT_ASSERT(v1 <= AddonVersion("2017.4.7b2"));
  CPPUNIT_ASSERT(v1 <= v1);
  CPPUNIT_ASSERT(AddonVersion("2017.4.7b2") > v1);
  CPPUNIT_ASSERT(AddonVersion("2017.4.7b2") >= v1);
  CPPUNIT_ASSERT(v1 >= v1);
  CPPUNIT_ASSERT(v1 != AddonVersion("2017.4.7b3"));

  CPPUNIT_ASSERT_EQUAL(v1.majorNumber(), 2017);
  CPPUNIT_ASSERT_EQUAL(v1.minorNumber(), 4);
  CPPUNIT_ASSERT_EQUAL(v1.patchLevel(), 7);
  CPPUNIT_ASSERT_EQUAL(v1.suffix(), suffix);

  // Round-trips std::string <-> AddonVersion
  CPPUNIT_ASSERT(AddonVersion("2017.4.7.dev13").str() == "2017.4.7.dev13");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7a2.dev8").str() == "2017.4.7a2.dev8");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7a2").str() == "2017.4.7a2");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7b2.dev5").str() == "2017.4.7b2.dev5");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7b2").str() == "2017.4.7b2");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7rc1.dev3").str() == "2017.4.7rc1.dev3");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7rc1").str() == "2017.4.7rc1");
  CPPUNIT_ASSERT(AddonVersion("2017.4.7").str() == "2017.4.7");

  // Check stream output
  std::ostringstream oss;
  oss << AddonVersion("2017.4.7b2.dev5");
  CPPUNIT_ASSERT(oss.str() == "2017.4.7b2.dev5");

  // Check ordering with all types of transitions, using operator<()
  auto checkStrictOrdering = [](const vector<AddonVersion>& v) {
    assert(v.size() > 1);
    for (std::size_t i=0; i < v.size() - 1; i++) {
      CPPUNIT_ASSERT(v[i] < v[i+1]);
    }
  };

  checkStrictOrdering({
      "3.12.8.dev1", "3.12.8.dev2", "3.12.8.dev12", "3.12.8a1.dev1",
      "3.12.8a1.dev2", "3.12.8a1", "3.12.8a2", "3.12.8b1.dev1",
      "3.12.8b1.dev2", "3.12.8b1", "3.12.8b2", "3.12.8b10",
      "3.12.8rc1.dev1", "3.12.8rc1.dev2", "3.12.8rc1.dev3",
      "3.12.8rc1", "3.12.8rc2", "3.12.8rc3", "3.12.8", "3.12.9.dev1",
      "3.12.9", "3.13.0", "4.0.0.dev1", "4.0.0.dev10", "4.0.0a1", "4.0.0",
      "2017.4.0", "2017.4.1", "2017.4.10", "2017.5.0", "2018.0.0"});

  FGTestApi::tearDown::shutdownTestGlobals();
}

void AddonManagementTests::testAddon()
{
    FGTestApi::setUp::initTestGlobals("Addon");

  std::string addonId = "org.FlightGear.addons.MyGreatAddon";
  Addon addon{addonId};
  addon.setVersion(AddonVersion("2017.2.5rc3"));
  addon.setBasePath(SGPath("/path/to/MyGreatAddon"));
  addon.setMinFGVersionRequired("2017.4.1");
  addon.setMaxFGVersionRequired("none");

  CPPUNIT_ASSERT_EQUAL(addon.getId(), addonId);
  CPPUNIT_ASSERT_EQUAL(*addon.getVersion(), AddonVersion("2017.2.5rc3"));
  CPPUNIT_ASSERT_EQUAL(addon.getBasePath(), SGPath("/path/to/MyGreatAddon"));
  CPPUNIT_ASSERT(addon.getMinFGVersionRequired() == "2017.4.1");

  const string refText = "addon '" + addonId + "' (version = 2017.2.5rc3, "
                         "base path = '/path/to/MyGreatAddon', "
                         "minFGVersionRequired = '2017.4.1', "
                         "maxFGVersionRequired = 'none')";
  CPPUNIT_ASSERT_EQUAL(addon.str(), refText);

  // Check stream output
  std::ostringstream oss;
  oss << addon;
  CPPUNIT_ASSERT_EQUAL(oss.str(), refText);

  // Set a max FG version and recheck
  addon.setMaxFGVersionRequired("2018.2.5");
  const string refText2 = "addon '" + addonId + "' (version = 2017.2.5rc3, "
                          "base path = '/path/to/MyGreatAddon', "
                          "minFGVersionRequired = '2017.4.1', "
                          "maxFGVersionRequired = '2018.2.5')";
  CPPUNIT_ASSERT(addon.getMaxFGVersionRequired() == "2018.2.5");
  CPPUNIT_ASSERT_EQUAL(addon.str(), refText2);

  FGTestApi::tearDown::shutdownTestGlobals();
}
