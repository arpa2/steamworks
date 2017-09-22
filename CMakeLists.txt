# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# Copyright (c) 2017, Adriaan de Groot <groot@kde.org>
#
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
#   SPDX-License-Identifier: BSD-2-Clause
#   License-Filename: LICENSE
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(steamworks C CXX)

find_package(ARPA2CM REQUIRED)
set (CMAKE_MODULE_PATH 
    ${CMAKE_MODULE_PATH}
    ${ARPA2CM_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake)

include(MacroAddUninstallTarget)
include(MacroCreateConfigFiles)
include(MacroEnsureOutOfSourceBuild)
include(MacroGitVersionInfo)

macro_ensure_out_of_source_build(
  "Do not build Steamworks in the source directory.")
get_version_from_git(Steamworks 0.92.0)

find_package(Quick-DER REQUIRED)

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
