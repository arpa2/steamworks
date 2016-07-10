# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake)

project(steamworks)

include(MacroEnsureOutOfSourceBuild)
include(CheckCXX11Flags)

MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
  "Do not build Steamworks in the source directory.")

message("   Using bundled libfcgi ...")
add_subdirectory(3rdparty/fcgi-2.4.0)
find_package(FCGI)

find_package(Log4cpp)
if(NOT LOG4CPP_FOUND)
  message("   Disable logging ...")
  add_definitions(-DNDEBUG)
  set(LOG4CPP_LIBRARIES "")
  set(LOG4CPP_INCLUDE_DIRS "")
  set(LOG4CPP_FOUND TRUE)
endif()

add_subdirectory(common)
add_subdirectory(crank)
add_subdirectory(shaft)
add_subdirectory(pulley)
