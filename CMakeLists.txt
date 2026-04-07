cmake_minimum_required(VERSION 3.25)

# Project configuration ---
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_STRING)
string(STRIP "${PROJECT_VERSION_STRING}" PROJECT_VERSION_STRING)

string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?" PROJECT_VERSION_NUMERIC "${PROJECT_VERSION_STRING}")

if (NOT PROJECT_VERSION_NUMERIC)
    message(FATAL_ERROR "Invalid VERSION file format: '${PROJECT_VERSION_STRING}'")
endif()

project(scanivalve-mps-protocol
    VERSION ${PROJECT_VERSION_NUMERIC}
    LANGUAGES C
)

add_library(scanivalve-mps-protocol INTERFACE)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/scanivalve/mps-protocol-version.h
    @ONLY
)

target_include_directories(scanivalve-mps-protocol
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

