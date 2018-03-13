include(FlightGearComponent)

set(SOURCES Addon.cxx
            AddonManager.cxx
            AddonMetadataParser.cxx
            AddonResourceProvider.cxx
            AddonVersion.cxx
            contacts.cxx
            exceptions.cxx
            )

set(HEADERS addon_fwd.hxx
            Addon.hxx
            AddonManager.hxx
            AddonMetadataParser.hxx
            AddonResourceProvider.hxx
            AddonVersion.hxx
            contacts.hxx
            exceptions.hxx
            pointer_traits.hxx
            )

flightgear_component(AddonManagement "${SOURCES}" "${HEADERS}")
