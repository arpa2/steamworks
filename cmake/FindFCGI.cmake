# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

# Use bundled FCGI
set(FCGI_LIBRARIES fcgi)
set(FCGI_INCLUDE_DIRS ${CMAKE_TOP_SOURCE_DIR}/3rdparty/fcgi-2.4.0/include)
set(FCGI_FOUND TRUE)

