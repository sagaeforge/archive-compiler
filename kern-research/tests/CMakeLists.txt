add_subdirectory(unit)

# Integration tests — run .kern files through the full compiler pipeline
add_test(
    NAME integration_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/integration/run_tests.sh
            $<TARGET_FILE:kernc>
            ${CMAKE_CURRENT_SOURCE_DIR}/integration
)
