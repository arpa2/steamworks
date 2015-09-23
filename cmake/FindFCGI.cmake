# Try to find FastCGI server libraries
#
include(FindPackageHandleStandardArgs)

find_library(_FCGI fcgi)
if (_FCGI)
  set(FCGI_LIBRARIES ${_FCGI})
endif()

find_path(FCGI_INCLUDE_DIRS fcgi_stdio.h)

find_package_handle_standard_args(FCGI 
  REQUIRED_VARS FCGI_LIBRARIES FCGI_INCLUDE_DIRS)

