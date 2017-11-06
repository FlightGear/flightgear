include(FlightGearComponent)

set(SOURCES Addon.cxx AddonManager.cxx AddonVersion.cxx)
set(HEADERS addon_fwd.hxx Addon.hxx AddonManager.hxx AddonVersion.hxx)

flightgear_component(AddonManagement "${SOURCES}" "${HEADERS}")

if (COMMAND flightgear_test)
  flightgear_test(test_AddonManagement test_AddonManagement.cxx)
endif()
