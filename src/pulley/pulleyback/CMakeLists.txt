# Copyright (c) 2016 InternetWide.org and the ARPA2.net project
# All rights reserved. See file LICENSE for exact terms (2-clause BSD license).
#
# Adriaan de Groot <groot@kde.org>

set(NULL_SRC
  null.c
  )

add_library(pulleyback_null MODULE ${NULL_SRC})
set_target_properties(pulleyback_null PROPERTIES PREFIX "")
