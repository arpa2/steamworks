# Copyright (c) 2014-2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# This is a simplified replacement for the libfcgi build system;
# it basically ignores everything except current Linux systems
# (on FreeBSD, use system package).

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

include_directories(include .)


set(LIBFCGI_SRC
  libfcgi/fcgiapp.c
  libfcgi/fcgio.cpp
  libfcgi/fcgi_stdio.c
  libfcgi/os_unix.c
  # libfcgi/os_win32.c
  )
add_library(fcgi STATIC ${LIBFCGI_SRC})
set_source_files_properties(libfcgi/os_unix.c PROPERTIES 
  COMPILE_FLAGS -D_POSIX_C_SOURCE=1)
set_source_files_properties(libfcgi/os_unix.c PROPERTIES 
  COMPILE_FLAGS -DHAVE_SOCKLEN=1)

set(CGI_FCGI_SRC
  cgi-fcgi/cgi-fcgi.c
  )
add_executable(cgi-fcgi ${CGI_FCGI_SRC})
target_link_libraries(cgi-fcgi fcgi)
