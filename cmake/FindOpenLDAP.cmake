# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# Try to find OpenLDAP client libraries. Sets standard variables
# OpenLDAP_LIBRARIES and OpenLDAP_INCLUDE_DIRS.
#
include(FindPackageHandleStandardArgs)

find_library(_LDAP ldap)
if (_LDAP)
  set(OpenLDAP_LIBRARIES ${_LDAP})
endif()

find_library(_LBER lber)
if (_LBER)
  set(OpenLDAP_BER_LIBRARIES ${_LBER})
endif()

find_path(OpenLDAP_INCLUDE_DIRS ldap.h)

find_package_handle_standard_args(OpenLDAP 
  REQUIRED_VARS OpenLDAP_LIBRARIES OpenLDAP_INCLUDE_DIRS)

