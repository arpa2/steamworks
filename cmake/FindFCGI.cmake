# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# Try to find FastCGI server libraries. Sets standard variables
# FCGI_LIBRARIES and FCGI_INCLUDE_DIRS.
#
include(FindPackageHandleStandardArgs)

find_library(_FCGI fcgi)
if (_FCGI)
  set(FCGI_LIBRARIES ${_FCGI})
endif()

find_path(FCGI_INCLUDE_DIRS fcgi_stdio.h)

find_package_handle_standard_args(FCGI 
  REQUIRED_VARS FCGI_LIBRARIES FCGI_INCLUDE_DIRS)

