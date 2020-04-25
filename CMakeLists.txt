# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# Copyright (c) 2017, Adriaan de Groot <groot@kde.org>
#
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
#   SPDX-License-Identifier: BSD-2-Clause
#   License-Filename: LICENSE
#
# Adriaan de Groot <groot@kde.org>
#
# Copyright 2019, Adriaan de Groot <groot@kde.org>
message(FATAL_ERROR "This repository has moved to GitLab\n\nGet Steamworks from\n\thttps://gitlab.com/arpa2/steamworks/\n")

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(steamworks C CXX)

include (FeatureSummary)

find_package (ARPA2CM 0.6 QUIET NO_MODULE)
set_package_properties (ARPA2CM PROPERTIES
    DESCRIPTION "CMake modules for ARPA2 projects"
    TYPE REQUIRED
    URL "https://github.com/arpa2/arpa2cm/"
    PURPOSE "Required for the CMake build system for ${PROJECT}"
)

if (ARPA2CM_FOUND)
    set (CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${ARPA2CM_MODULE_PATH}
        ${CMAKE_SOURCE_DIR}/cmake)
else()
    feature_summary (WHAT ALL)
    message (FATAL_ERROR "ARPA2CM is required.")
endif()

include(MacroAddUninstallTarget)
include(MacroCreateConfigFiles)
include(MacroEnsureOutOfSourceBuild)
include(MacroGitVersionInfo)

macro_ensure_out_of_source_build(
  "Do not build Steamworks in the source directory.")
get_version_from_git(Steamworks 0.93)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
add_subdirectory(src)
add_uninstall_target()

#
# PACKAGING
#

set(CPACK_PACKAGE_NAME "Steamworks")
set(CPACK_PACKAGE_VERSION ${Steamworks_VERSION})
set(CPACK_PACKAGE_VENDOR "ARPA2.net")
set(CPACK_PACKAGE_CONTACT "Adriaan de Groot <groot@kde.org>")
set(CPACK_FREEBSD_PACKAGE_LICENSE "BSD-2-Clause")

include(PackAllPossible)
include(CPack)

create_config_files(Steamworks)

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)
