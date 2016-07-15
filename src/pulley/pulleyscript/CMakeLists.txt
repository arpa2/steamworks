# Copyright (c) 2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# This is the CMake-ified version of the pulleyscript Makefile

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../../cmake)

find_package(SQLite3 REQUIRED)
find_package(BISON 3 REQUIRED)
find_package(FLEX REQUIRED)

# The Bison / Flex bits are a bit particular;
# there's a parser.h in the source-dir that
# needs to be included, and the outputs go
# into the source-dir by default but are looked-for
# in the binary-dir.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
bison_target(bpulley pulley.yacc ${CMAKE_CURRENT_BINARY_DIR}/pulley.tab.cacc
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/pulley.tab.hahh)
flex_target(fpulley pulley.lex ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
add_flex_bison_dependency(fpulley bpulley)

set(PSLIB_SRC
  bitset.c
  condition.c
  driver.c
  generator.c
  lexhash.c
  resist.c
  squeal.c
  variable.c
  ${FLEX_fpulley_OUTPUTS})

set(PSPPLIB_SRC
  parserpp.cpp
  bindingpp.cpp
  )

set(COMPILER_SRC
  compiler.c
  )

set(SIMPLE_SRC
  simple.cpp
  )

# Adding -DPULLEY_SQUEAL_DIR will change where the Squeal databases are created.
#
# add_definitions(-DPULLEY_SQUEAL_DIR="${CMAKE_INSTALL_PREFIX}/var/db/")
include_directories(${SQLITE3_INCLUDE_DIRS} ${FLEX_INCLUDE_DIRS})
add_library(pslib STATIC ${PSLIB_SRC})
add_library(pspplib STATIC ${PSPPLIB_SRC})

add_executable(compiler ${COMPILER_SRC})
target_link_libraries(compiler pslib ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES})

add_executable(simple ${SIMPLE_SRC})
target_link_libraries(simple pspplib pslib swcommon ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES} ${LOG4CPP_LIBRARIES})
