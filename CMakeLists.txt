# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# Copyright (c) 207, Adriaan de Groot <groot@kde.org>
#
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

project(steamworks)

include(MacroEnsureOutOfSourceBuild)
include(CheckCXX11Flags)
include(CheckC99Flags)

MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
  "Do not build Steamworks in the source directory.")

add_subdirectory(src)

