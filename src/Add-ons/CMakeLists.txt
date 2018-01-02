include(FlightGearComponent)

set(SOURCES Addon.cxx
            AddonManager.cxx
            AddonVersion.cxx
            contacts.cxx
            exceptions.cxx
            )

set(HEADERS addon_fwd.hxx
            Addon.hxx
            AddonManager.hxx
            AddonVersion.hxx
            contacts.hxx
            exceptions.hxx
            pointer_traits.hxx
            )

flightgear_component(AddonManagement "${SOURCES}" "${HEADERS}")

if (COMMAND flightgear_test)
  flightgear_test(test_AddonManagement test_AddonManagement.cxx)
endif()
