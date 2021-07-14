cmake_minimum_required(VERSION 3.19)
project(ThreadSafeSmartPointers)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_UNIT_TESTS "Build unit tests." NO)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/bin/libs)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/bin/libs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/bin)

########################################################################################################################
# Unit tests.
########################################################################################################################
IF (BUILD_UNIT_TESTS)
    add_subdirectory ("tests")
ENDIF()

########################################################################################################################
# Main sources.
########################################################################################################################
add_subdirectory("include")
