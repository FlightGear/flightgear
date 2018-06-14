# Add each system test category.
foreach( system_test_category
        FDM
        Instrumentation
        Navaids
    )

    add_subdirectory(${system_test_category})

endforeach( system_test_category )


set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    PARENT_SCOPE
)


set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    PARENT_SCOPE
)
