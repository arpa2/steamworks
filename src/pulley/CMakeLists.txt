# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

include_directories(
  ${FCGI_INCLUDE_DIRS} 
  ${OpenLDAP_INCLUDE_DIR} 
  ${LOG4CPP_INCLUDE_DIRS} 
  ../common 
  ../3rdparty)

add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

add_subdirectory(pulleyscript)
add_subdirectory(pulleyback)

set(PULLEY_SRC
  main.cpp
  pulley.cpp
  )

add_executable(pulley ${PULLEY_SRC})
target_link_libraries(pulley 
  ${OpenLDAP_LIBRARIES} ${OpenLDAP_BER_LIBRARIES} 
  ${FCGI_LIBRARIES} 
  ${LOG4CPP_LIBRARIES}
  swldap 
  swcommon 
  pspplib 
  pslib
  )
set_target_properties(pulley PROPERTIES LINK_FLAGS -rdynamic)

###
#
# pulleyback_test is purely a developer tool, allowing load-testing
# of a given pulley backend. It is not installed.
#
# Adding -DNO_SECURITY will allow the pulleyback_test to load a library
# from any given path, not just paths inside the PREFIX/share/steamworks/pulleyback/
# directory.
#
# add_definitions(-DNO_SECURITY=1)
add_executable(pulleyback_test pulleyback_test.cpp)
target_link_libraries(pulleyback_test 
  pspplib 
  pslib 
  swcommon
  )
# We need to link this executable such that the plugins can get
# at the write_logger() symbol from the executable -- otherwise
# the plugin can't produce logging output.
set_target_properties(pulleyback_test PROPERTIES LINK_FLAGS -rdynamic)

install(TARGETS pulley
  RUNTIME DESTINATION bin
  )
install(FILES pulleyback.h
  DESTINATION include/steamworks/
  )
