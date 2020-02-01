# Add each simgear test category.
foreach( simgear_test_category
    )

    add_subdirectory(${simgear_test_category})

endforeach( simgear_test_category )


set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    PARENT_SCOPE
)


set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    PARENT_SCOPE
)
