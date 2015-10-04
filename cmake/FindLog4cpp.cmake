# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# Try to find log4cpp libraries. Sets standard variables
# LOG4CPP_LIBRARIES and LOG4CPP_INCLUDE_DIRS.
#
include(FindPackageHandleStandardArgs)

find_library(_L4CPP log4cpp)
if (_L4CPP)
  set(LOG4CPP_LIBRARIES ${_L4CPP})
endif()

find_path(LOG4CPP_INCLUDE_DIRS log4cpp/Category.hh)

find_package_handle_standard_args(LOG4CPP 
  REQUIRED_VARS LOG4CPP_LIBRARIES LOG4CPP_INCLUDE_DIRS)

