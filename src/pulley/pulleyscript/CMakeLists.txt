# Copyright (c) 2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# This is the CMake-ified version of the pulleyscript Makefile

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../../cmake)

include(CheckFunctionExists)
include(CheckSymbolExists)

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

check_function_exists(dlfunc HAVE_FUN_DLFUNC)
if(HAVE_FUN_DLFUNC)
  add_definitions(-DHAVE_DLFUNC)
endif()

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
  backend.cpp
  bindingpp.cpp
  parserpp.cpp
  )
if(NOT HAVE_FUN_DLFUNC)
  set(PSPPLIB_SRC ${PSPPLIB_SRC} dlfunc.c)
endif()

set(COMPILER_SRC
  compiler.c
  logger.c
  )

set(SIMPLE_SRC
  simple.cpp
  )

# Adding -DPULLEY_SQUEAL_DIR will change where the Squeal databases are created.
# Adding -DPULLEY_BACKEND_DIR will change where the Pulley backends are searched.
#
if(NOT "${PULLEY_SQUEAL_DIR}" STREQUAL "")
  message("   Using SQL directory '${PULLEY_SQUEAL_DIR}'")
  add_definitions(-DPULLEY_SQUEAL_DIR="${PULLEY_SQUEAL_DIR}")
endif()
if(NOT "${PULLEY_BACKEND_DIR}" STREQUAL "")
  message("   Using backend directory '${PULLEY_BACKEND_DIR}'")
  add_definitions(-DPULLEY_BACKEND_DIR="${PULLEY_BACKEND_DIR}")
endif()


include_directories(${SQLITE3_INCLUDE_DIRS} ${FLEX_INCLUDE_DIRS})
add_library(pslib STATIC ${PSLIB_SRC})
add_library(pspplib STATIC ${PSPPLIB_SRC})

check_symbol_exists(dlclose dlfcn.h HAVE_FUN_DLCLOSE)
if(NOT HAVE_FUN_DLCLOSE)
  message("   .. Checking for dlclose in -ldl")
  set(_CRL ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES -ldl)
  check_symbol_exists(dlclose dlfcn.h HAVE_FUN_DLCLOSE2)
  if(HAVE_FUN_DLCLOSE2)  # Now it needs -ldl
    target_link_libraries(pspplib dl)
  endif()
endif()

add_executable(compiler ${COMPILER_SRC})
target_link_libraries(compiler pslib ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES})

add_executable(simple ${SIMPLE_SRC})
target_link_libraries(simple pspplib pslib swcommon ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES} ${LOG4CPP_LIBRARIES})
set_target_properties(simple PROPERTIES LINK_FLAGS -rdynamic)
