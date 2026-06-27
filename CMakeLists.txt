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

option(MPS_BUILD_INSTALL "Enable install rules" OFF)

add_library(scanivalve-mps-protocol INTERFACE)
add_library(scanivalve::mps-protocol ALIAS scanivalve-mps-protocol)

target_compile_features(aether INTERFACE c_std_11)

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

if (MPS_BUILD_INSTALL)
    include(GNUInstallDirs)

    install(DIRECTORY include/scanivalve DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/scanivalve/mps-protocol-version.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/scanivalve
    )
    install(FILES README.md LICENSE VERSION DESTINATION .)
endif()
