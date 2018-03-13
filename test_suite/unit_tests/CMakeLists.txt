# Add each unit test category.
foreach( unit_test_category
        Add-ons
        general
        FDM
        Navaids
        Scripting
    )

    add_subdirectory(${unit_test_category})

endforeach( unit_test_category )


set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    PARENT_SCOPE
)


set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    PARENT_SCOPE
)
