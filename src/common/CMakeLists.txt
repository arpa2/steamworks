# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../cmake)

project(common)

find_package(Log4cpp)
find_package(FCGI REQUIRED)
find_package(OpenLDAP REQUIRED)

include_directories(${FCGI_INCLUDE_DIRS} ../3rdparty)

set(SWLDAP_SRC
  swldap/connection.cpp
  swldap/search.cpp
  swldap/serverinfo.cpp
  swldap/sync.cpp
  swldap/private.cpp
  )

add_library(swldap STATIC ${SWLDAP_SRC})
target_link_libraries(swldap ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${LOG4CPP_LIBRARIES})

set(SWCOMMON_SRC
  fcgi.cpp
  jsonresponse.cpp
  logger.cpp
  )

add_library(swcommon STATIC ${SWCOMMON_SRC})
target_link_libraries(swcommon ${LOG4CPP_LIBRARIES} ${FCGI_LIBRARIES})

