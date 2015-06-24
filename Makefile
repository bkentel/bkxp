ifndef CXX
	CXX = g++
endif

RM  = rm -f

CPPFLAGS = $(shell root-config --cflags)
LDFLAGS  = $(shell root-config --ldflags)
LDLIBS   = $(shell root-config --libs)

SRC_DIR_BKXP        = src
SRC_DIR_BKLIB       = src/bklib
SRC_DIR_BKXP_TESTS  = test
SRC_DIR_BKLIB_TESTS = test/bklib

SRCS  = $(wildcard $(SRC_DIR_BKXP)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKLIB)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKXP_TESTS)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKLIB_TESTS)/*.cpp)

OBJS = $(subst .cpp,.o,$(SRCS))

all: bkxp

tool: $(OBJS)
	$(CXX) $(LDFLAGS) -o bkxp $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

include .depend
