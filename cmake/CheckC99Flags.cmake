# Check which flag to pass in for C C99 support.
#
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-std=c99" COMPILER_SUPPORTS_C99)
CHECK_C_COMPILER_FLAG("-std=gnu99" COMPILER_SUPPORTS_G99)
if(COMPILER_SUPPORTS_G99)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
elseif(COMPILER_SUPPORTS_C99)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
else()
        message(FATAL "The compiler ${CMAKE_C_COMPILER} has no C99 support. Please use a different C compiler.")
endif()
