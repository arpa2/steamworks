# Copyright (c) 2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>
#
# Copyright 2019, Adriaan de Groot <groot@kde.org>

# This is the CMake-ified version of the pulleyscript Makefile

include(CheckFunctionExists)
include(CheckSymbolExists)

find_package(SQLite3)
set_package_properties (SQLite3 PROPERTIES
    DESCRIPTION "SQLite database"
    TYPE REQUIRED
    URL "https://sqlite.org/index.html"
    PURPOSE "In-memory database of tuples"
)

find_package(BISON 3)
set_package_properties (BISON PROPERTIES
    DESCRIPTION "Bison parser generator"
    TYPE REQUIRED
    URL "https://www.gnu.org/software/bison/"
    PURPOSE "Parsing pulleyscript statements"
)

find_package(FLEX)
set_package_properties (FLEX PROPERTIES
    DESCRIPTION "FLEX lexical analyser"
    TYPE REQUIRED
    URL "https://www.gnu.org/software/flex/"
    PURPOSE "Parsing pulleyscript statements"
)

include(CheckQSortR)

# The Bison / Flex bits are a bit particular;
# there's a parser.h in the source-dir that
# needs to be included, and the outputs go
# into the source-dir by default but are looked-for
# in the binary-dir.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if (BISON_FOUND)
    bison_target(bpulley pulley.yacc ${CMAKE_CURRENT_BINARY_DIR}/pulley.tab.cacc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/pulley.tab.hahh)
    flex_target(fpulley pulley.lex ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
    add_flex_bison_dependency(fpulley bpulley)
endif()

check_function_exists(dlfunc HAVE_FUN_DLFUNC)
if(HAVE_FUN_DLFUNC)
  add_definitions(-DHAVE_DLFUNC)
endif()

include_directories(${SQLITE3_INCLUDE_DIRS} ${FLEX_INCLUDE_DIRS})

### SOURCES
#
# The sources for the different targets. Note that pslib_o
# is a CMake object-library, and that squeal.c is compiled
# separately for the static library and for the two testing
# executables, with different options.
#
set(PSLIB_SRC
  bitset.c
  condition.c
  driver.c
  generator.c
  lexhash.c
  resist.c
  variable.c
  ${FLEX_fpulley_OUTPUTS})

add_library(pslib_o OBJECT ${PSLIB_SRC})
target_compile_definitions(pslib_o PUBLIC -D_GNU_SOURCE=1)
if(QSORT_IS_GLIBC)
  target_compile_definitions(pslib_o PUBLIC -DQSORT_IS_GLIBC=1)
endif()

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

add_library(pslib STATIC $<TARGET_OBJECTS:pslib_o>  squeal.c)

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

add_executable(compiler $<TARGET_OBJECTS:pslib_o> squeal.c ${COMPILER_SRC})
target_compile_definitions(compiler PUBLIC -DALLOW_INSECURE_DB)
target_link_libraries(compiler ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES})

add_executable(simple $<TARGET_OBJECTS:pslib_o> squeal.c ${SIMPLE_SRC})
target_compile_definitions(simple PUBLIC -DALLOW_INSECURE_DB)
target_link_libraries(simple pspplib swcommon ${SQLITE3_LIBRARIES} ${FLEX_LIBRARIES} ${LOG4CPP_LIBRARIES})
set_target_properties(simple PROPERTIES LINK_FLAGS -rdynamic)

# Try to compile all of the sample scripts
file(GLOB scriptfiles LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.ply)
foreach(script ${scriptfiles})
    add_test(
        NAME compile-${script}
        COMMAND compiler -S ${CMAKE_CURRENT_BINARY_DIR}/ ${script}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endforeach()
