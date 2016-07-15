# Copyright (c) 2014, 2015 InternetWide.org and the ARPA2.net project
# All rights reserved.
#
# See the LICENSE file for details.
#
# Top-level bogus makefile for the Steamworks project. The real
# build-system is CMake. This makefile just arranges for an out-
# of-source build in a new subdirectory.
#
PREFIX ?= /usr/local

all: build

check-build:
	test -d build/ || mkdir build
	test -d build

check-cmake: check-build
	test -f build/Makefile || ( cd build ; cmake -DCMAKE_INSTALL_PREFIX:PATH=$(PREFIX) ../src )
	test -f build/Makefile

build: check-cmake
	( cd build ; $(MAKE) )

install: build
	( cd build ; $(MAKE) install PREFIX=$PREFIX )
