# Try to find log4cpp libraries
#
include(FindPackageHandleStandardArgs)

find_library(_L4CPP log4cpp)
if (_L4CPP)
  set(LOG4CPP_LIBRARIES ${_L4CPP})
endif()

find_path(LOG4CPP_INCLUDE_DIRS log4cpp/Category.hh)

find_package_handle_standard_args(LOG4CPP 
  REQUIRED_VARS LOG4CPP_LIBRARIES LOG4CPP_INCLUDE_DIRS)

