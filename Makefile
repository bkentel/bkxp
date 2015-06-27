ifndef CXX
	CXX = g++
endif

RM  = rm -f

#CPPFLAGS = $(shell root-config --cflags)
#LDFLAGS  = $(shell root-config --ldflags)
#LDLIBS   = $(shell root-config --libs)

CPPFLAGS += -std=c++14

SRC_DIR_BKXP        = src
SRC_DIR_BKLIB       = src/bklib
SRC_DIR_BKXP_TESTS  = test
SRC_DIR_BKLIB_TESTS = test/bklib

INCLUDE  = -isystem ./deps/core/include
INCLUDE += -isystem ./deps/config/include
INCLUDE += -isystem ./deps/preprocessor/include
INCLUDE += -isystem ./deps/assert/include
INCLUDE += -isystem ./deps/static_assert/include
INCLUDE += -isystem ./deps/type_traits/include
INCLUDE += -isystem ./deps/mpl/include
INCLUDE += -isystem ./deps/integer/include
INCLUDE += -isystem ./deps/throw_exception/include
INCLUDE += -isystem ./deps/smart_ptr/include
INCLUDE += -isystem ./deps/exception/include
INCLUDE += -isystem ./deps/predef/include
INCLUDE += -isystem ./deps/utility/include
INCLUDE += -isystem ./deps/tuple/include
INCLUDE += -isystem ./deps/random/include
INCLUDE += -isystem ./deps/Catch/include
INCLUDE += -isystem ./deps/pcg-cpp/include
INCLUDE += -isystem ./deps/rapidjson/include
INCLUDE += -iquote ./$(SRC_DIR_BKXP)

CPPFLAGS += $(INCLUDE)

CPPFLAGS += -D BK_NO_SDL -D BK_NO_PCH -D BK_TESTS_ONLY

ifeq ($(findstring g++,$(CXX)),g++)
  CPPFLAGS += -Wredundant-decls -Wcast-align -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Wextra -Wall -Werror -Winvalid-pch -Wredundant-decls -Wmissing-prototypes -Wformat=2 -Wmissing-format-attribute -Wformat-nonliteral 
else
  CPPFLAGS += -Weverything
endif

SRCS  = $(wildcard $(SRC_DIR_BKXP)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKLIB)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKXP_TESTS)/*.cpp)
SRCS += $(wildcard $(SRC_DIR_BKLIB_TESTS)/*.cpp)

OBJS = $(subst .cpp,.o,$(SRCS))

all: bkxp

bkxp: $(OBJS)
	$(CXX) $(LDFLAGS) -o bkxp $(OBJS) $(LDLIBS)

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^ >> ./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

include .depend
