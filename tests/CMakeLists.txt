# Tests CMakeLists.txt
# Build with: cmake .. -DBUILD_TESTS=ON && make

# Get parent project's library name
get_directory_property(PARENT_LIB_NAME DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.. DEFINITION LIB_NAME)

# Test executables
set(TEST_SOURCES
    test_crc.cpp
    test_tcp.cpp
    test_udp.cpp
    test_serial.cpp
    test_http.cpp
)

foreach(test_source ${TEST_SOURCES})
    # Get test name from filename
    get_filename_component(test_name ${test_source} NAME_WE)

    # Create executable
    add_executable(${test_name} ${test_source})

    # Link against the library
    target_link_libraries(${test_name} PRIVATE ${PARENT_LIB_NAME})

    # Include directories
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Add pthread on Unix
    if(UNIX)
        target_link_libraries(${test_name} PRIVATE pthread)
    endif()

    # Compiler options
    target_compile_options(${test_name} PRIVATE -Wall -Wextra)
endforeach()

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_crc
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_tcp
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_udp
    COMMENT "Running unit tests..."
    DEPENDS test_crc test_tcp test_udp
)

# Note: test_serial and test_http require external resources
# and are not included in the default test run
