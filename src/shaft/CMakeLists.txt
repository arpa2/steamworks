# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

find_package(Log4cpp)
find_package(FCGI REQUIRED)
find_package(OpenLDAP REQUIRED)

include_directories(
  ${FCGI_INCLUDE_DIRS} 
  ${OpenLDAP_INCLUDE_DIR} 
  ${LOG4CPP_INCLUDE_DIRS} 
  ../common 
  ../3rdparty)

set(SHAFT_SRC
  main.cpp
  shaft.cpp
  )

add_executable(shaft ${SHAFT_SRC})
target_link_libraries(shaft ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${FCGI_LIBRARIES} ${LOG4CPP_LIBRARIES} swldap swcommon)

install(TARGETS shaft
  RUNTIME DESTINATION bin
  )
