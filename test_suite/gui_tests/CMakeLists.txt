# Add each GUI test category.
foreach( gui_test_category
    )

    add_subdirectory(${gui_test_category})

endforeach( gui_test_category )


set(TESTSUITE_SOURCES
    ${TESTSUITE_SOURCES}
    PARENT_SCOPE
)


set(TESTSUITE_HEADERS
    ${TESTSUITE_HEADERS}
    PARENT_SCOPE
)
