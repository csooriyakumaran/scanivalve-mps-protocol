cmake_minimum_required(VERSION 3.25)
project(scanivalve-mps-protocol C)

add_library(scanivalve-mps-protocol INTERFACE)

target_include_directories(scanivalve-mps-protocol
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
