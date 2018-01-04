# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>
include_directories(
  ${FCGI_INCLUDE_DIRS} 
  ${OpenLDAP_INCLUDE_DIR} 
  ${LOG4CPP_INCLUDE_DIRS} 
  ../3rdparty)

set(SWLDAP_SRC
  swldap/connection.cpp
  swldap/search.cpp
  swldap/serverinfo.cpp
  swldap/sync.cpp
  swldap/update.cpp
  swldap/private.cpp
  )

add_library(swldap STATIC ${SWLDAP_SRC})
target_link_libraries(swldap ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${LOG4CPP_LIBRARIES})

set(SWCOMMON_SRC
  fcgi.cpp
  jsonresponse.cpp
  logger.cpp
  verb.cpp
  )

add_library(swcommon STATIC ${SWCOMMON_SRC})
target_link_libraries(swcommon ${LOG4CPP_LIBRARIES} ${FCGI_LIBRARIES})

