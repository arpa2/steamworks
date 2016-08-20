# Copyright (c) 2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# Discover the order of parameters for qsort_r().
# Sets QSORT_IS_GLIBC (Linux GLibC parameter order)
# if the parameter order is func, thunk (as in GLibC)
# and sets it to false if not (e.g. FreeBSD).
#
try_run(
  _check_qsort_result
  QSORT_COMPILE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/check_qsort_r.c
  )
if(NOT QSORT_COMPILE)
  message(FATAL "Could not compile test program for qsort_r()")
endif()
# _check_qsort_result is the exit-code from the program, which is 0
# when GLibC parameter-order is used.
if(NOT _check_qsort_result)
  message("   qsort_r() has GLibC parameter order (func, thunk)")
  set(QSORT_IS_GLIBC 1)
else()
  message("   qsort_r() has FreeBSD parameter order (thunk, func)")
  set(QSORT_IS_GLIBC 0)
endif()

