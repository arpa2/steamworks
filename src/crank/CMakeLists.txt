# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../../cmake)

project(crank)

find_package(Log4cpp)
find_package(FCGI REQUIRED)
find_package(OpenLDAP REQUIRED)

include_directories(${LOG4CPP_INCLUDE_DIRS} ${FCGI_INCLUDE_DIRS} ../common ../3rdparty)

set(CRANK_SRC
  main.cpp
  crank.cpp
  )

add_executable(crank ${CRANK_SRC})
target_link_libraries(crank ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${FCGI_LIBRARIES} ${LOG4CPP_LIBRARIES} swldap swcommon)

