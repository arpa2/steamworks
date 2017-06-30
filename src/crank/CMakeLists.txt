# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

include_directories(${FCGI_INCLUDE_DIRS} ${LOG4CPP_INCLUDE_DIRS} ../common ../3rdparty)

set(CRANK_SRC
  main.cpp
  crank.cpp
  )

add_executable(crank ${CRANK_SRC})
target_link_libraries(crank ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} ${FCGI_LIBRARIES} ${LOG4CPP_LIBRARIES} swldap swcommon)

install(TARGETS crank
  RUNTIME DESTINATION bin
  )
