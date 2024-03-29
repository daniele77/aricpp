################################################################################
# ARICPP - ARI interface for C++
# Copyright (C) 2017-2021 Daniele Pallastrelli
#
# Boost Software License - Version 1.0 - August 17th, 2003
#
# Permission is hereby granted, free of charge, to any person or organization
# obtaining a copy of the software and accompanying documentation covered by
# this license (the "Software") to use, reproduce, display, distribute,
# execute, and transmit the Software, and to prepare derivative works of the
# Software, and to permit third-parties to whom the Software is furnished to
# do so, all subject to the following:
#
# The copyright notices in the Software and this entire statement, including
# the above license grant, this restriction and the following disclaimer,
# must be included in all copies of the Software, in whole or in part, and
# all derivative works of the Software, unless such copies or derivative
# works are solely in the form of machine-executable object code generated by
# a source language processor.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
# FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
################################################################################

cmake_minimum_required(VERSION 3.8)

project(aricpp VERSION 1.1.3 LANGUAGES CXX)

include(GNUInstallDirs)

option(ARICPP_BuildExamples "Build the examples." OFF)


if(WIN32)
    macro(get_WIN32_WINNT version)
        if(CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif()
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif()
    endmacro()

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif()

set(Boost_NO_BOOST_CMAKE ON)
add_definitions( -DBOOST_ALL_NO_LIB ) # for windows
find_package(Boost 1.66 REQUIRED COMPONENTS system)

find_package(Threads REQUIRED)

# Add Library
add_library(aricpp INTERFACE)
# Add target alias
add_library(aricpp::aricpp ALIAS aricpp)

target_include_directories(aricpp
    INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(aricpp INTERFACE Threads::Threads)
target_link_libraries(aricpp INTERFACE Boost::system)
target_compile_definitions(aricpp INTERFACE BOOST_ASIO_NO_DEPRECATED=1)
target_compile_features(aricpp INTERFACE cxx_std_14)

# Examples
if (ARICPP_BuildExamples)
    add_subdirectory(examples)
endif()

# Install
install(DIRECTORY include/aricpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "aricppConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/aricppConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/aricpp"
)

# Generate pkg-config .pc file
set(PKGCONFIG_INSTALL_DIR
    ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
    CACHE PATH "Installation directory for pkg-config (aricpp.pc) file"
)
configure_file(
    "aricpp.pc.in"
    "aricpp.pc"
    @ONLY
)

install(TARGETS aricpp EXPORT aricppTargets LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT aricppTargets FILE aricppTargets.cmake NAMESPACE aricpp:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/aricpp)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/aricppConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/aricpp"
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/aricpp.pc"
    DESTINATION ${PKGCONFIG_INSTALL_DIR}
)
