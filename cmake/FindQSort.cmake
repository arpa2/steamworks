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
  QSORT_IS_GLIBC
  QSORT_COMPILE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/check_qsort_r.c
  RUN_OUTPUT_VARIABLE QSORT_OUTPUT
  )
if(NOT QSORT_COMPILE)
  message(FATAL "Could not compile test program for qsort_r()")
endif()
# QSORT_IS_GLIBC is the exit-code from the program, which is 0
# when GLibC parameter-order is used.
if(NOT QSORT_IS_GLIBC)
  message("   qsort_r() has GLibC parameter order (func, thunk) ${QSORT_OUTPUT}")
else()
  message("   qsort_r() has FreeBSD parameter order (thunk, func) ${QSORT_OUTPUT}")
endif()

