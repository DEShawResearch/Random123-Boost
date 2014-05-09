#!/usr/bin/env garden-exec
#
# At DESRES we have some infrastructure that allows us to
# prepend a header that sets up the environment for programs written
# in scripting languages.  One nifty use case is executable Makefiles
# such as this one.
#
# Outside of DESRES, just do whatever you need to set the standard
# Make variables:  CXX, CPPFLAGS, CXXFLAGS, CFLAGS, LDFLAGS, LDLIBS, etc. before
# running make.
#{
# garden env-keep-only PREFIX
#
# garden load \
#   make/3.81-11A/bin \
#   gcc/4.8.2-27B/bin \
#   TestU01/1.2.3-02A/lib \
#   boost/1_55_0-30A/Release/lib
#
# # Uncomment these lines to use icpc
# #export newgcc=$(type -p gcc)
# #echo newgcc=$newgcc
# #garden load gcc/4.4.4-21A/bin \
# #            intel/ccompxe/2011.2.137-05/icc/bin
# #export CXX=icpc
#
# garden strip-colons
#
# make -f "$0" "$@"
#}
#

All:

## Tell GNU make that we want to link with CXX rather than CC.
LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

CPPFLAGS+= -I../../..
LDLIBS+= -lboost_unit_test_framework # -lgmpxx -lgmp #-lcrypto

ifeq ($(CXX),icpc)
# To get icpc to work with boost we have to link against
# an up-to-date libstdc++.so, but we have to compile with
# an out-of-date include/ STL directory because icpc doesn't
# grok the contents of the latest g++ STL includes.
CXXFLAGS+=-ggdb -fast
CXXFLAGS+=-diag-disable 279 # 'controlling expression is a constant'
gcclib=$(shell $(newgcc) --print-file-name=libstdc++.so)
LDFLAGS+=-Wl,-rpath=$(dir $(gcclib))
LDLIBS+=$(gcclib)
else # g++

OPTIMIZE= -ggdb3 -O3
COMMONFLAGS=-Wall $(OPTIMIZE) -march=native -Wno-unused-local-typedefs
CXXFLAGS+=$(COMMONFLAGS) -std=c++98
CFLAGS+=$(COMMONFLAGS) -std=gnu99

endif

%.s : %.cpp
	$(CXX) -S $(filter-out %gdb,$(CXXFLAGS) $(CPPFLAGS)) $<

%.s : %.c
	$(CC) -S $(filter-out %gdb,$(CFLAGS) $(CPPFLAGS)) $<

HDRS:=../../../boost/random/*.hpp ../../../boost/random/detail/*.hpp *.hpp *.ipp Makefile

Binaries:=$(basename $(wildcard test_*.cpp))

All: $(Binaries)
$(Binaries) : % : %.o
$(Binaries:%=%.o): $(HDRS)
$(Binaries:%=%.s): $(HDRS)

$(filter test_%,$(Binaries:%=%.o)): override CPPFLAGS+=-DBOOST_TEST_DYN_LINK

test_philox2x64.o : override COMMONFLAGS+=  -Wno-sequence-point -Wno-strict-aliasing

.PHONY: test-env
test-env:
	@echo LDFLAGS: $(LDFLAGS) 
	@echo CFLAGS: $(CFLAGS) 
	@echo CXXFLAGS: $(CXXFLAGS)
