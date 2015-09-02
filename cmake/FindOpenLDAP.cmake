# Try to find OpenLDAP client libraries
#
include(FindPackageHandleStandardArgs)

find_library(_LDAP ldap)
if (_LDAP)
  set(OpenLDAP_LIBRARIES ${_LDAP})
endif()

find_path(OpenLDAP_INCLUDE_DIRS ldap.h)

find_package_handle_standard_args(OpenLDAP 
  REQUIRED_VARS OpenLDAP_LIBRARIES OpenLDAP_INCLUDE_DIRS)

