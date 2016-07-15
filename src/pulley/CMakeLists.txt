# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../cmake)

project(pulley)

find_package(Log4cpp)
find_package(FCGI REQUIRED)
find_package(OpenLDAP REQUIRED)

find_package(FLEX REQUIRED)  # Required by pulleyscript
find_package(SQLite3 REQUIRED)

include_directories(${FCGI_INCLUDE_DIRS} ${LOG4CPP_INCLUDE_DIRS} ../common ../3rdparty)

add_subdirectory(pulleyscript)
add_subdirectory(pulleyback)

set(PULLEY_SRC
  backend.cpp
  main.cpp
  pulley.cpp
  )

add_executable(pulley ${PULLEY_SRC})
target_link_libraries(pulley ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${FCGI_LIBRARIES} ${LOG4CPP_LIBRARIES})
target_link_libraries(pulley swldap swcommon pspplib pslib)
target_link_libraries(pulley ${FLEX_LIBRARIES} ${SQLITE3_LIBRARIES})

add_definitions(-DNO_SECURITY=1)
add_executable(pulleyback_test backend.cpp pulleyback_test.cpp)
target_link_libraries(pulleyback_test swcommon)
# We need to link this executable such that the plugins can get
# at the write_logger() symbol from the executable -- otherwise
# the plugin can't produce logging output.
set_target_properties(pulleyback_test PROPERTIES LINK_FLAGS -rdynamic)

install(TARGETS pulley
  RUNTIME DESTINATION bin
  )
