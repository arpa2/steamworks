# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# Copyright (c) 207, Adriaan de Groot <groot@kde.org>
#
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

project(steamworks)

include(MacroAddUninstallTarget)
include(MacroCreateConfigFiles)
include(MacroEnsureOutOfSourceBuild)
include(MacroGitVersionInfo)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro_ensure_out_of_source_build(
  "Do not build Steamworks in the source directory.")

get_version_from_git(Steamworks 0.92.0)

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
