# Taken largely from http://scottmcpeak.com/autodepend/autodepend.html


OPENMP_FLAGS := -fopenmp
# Note: use of "=" instead of ":=" was intentional. This allows Makefile.user
# to override OPENMP_FLAGS.
CXXFLAGS      = -std=c++11 -Wall -Wextra -pedantic $(OPENMP_FLAGS)
LIBFLAGS     := 

OBJS := $(patsubst %.cpp,%.o, $(wildcard *.cpp))

OPTIMIZATIONS := -O2 -DNDEBUG -flto

ROUTER=ROUTE.exe

all: release


debug: CXXFLAGS += -g
debug: $(ROUTER)

profile: CXXFLAGS += $(OPTIMIZATIONS) -g -pg
profile: $(ROUTER)

release: CXXFLAGS+= $(OPTIMIZATIONS)
release: $(ROUTER)

# link
$(ROUTER): $(OBJS) main.o
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBFLAGS) -o $(ROUTER)

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)
-include $(TESTOBJS:.o=.d)

# For if we used precomipled headers later
# precomp.hpp.gch: precomp.hpp
# 	$(CXX) $(CXXFLAGS) precomp.hpp

# compile and generate dependency info;
# more complicated dependency computation, so all prereqs listed
# will also become command-less, prereq-less targets
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
# %.o: precomp.hpp.gch %.cpp
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

# remove compilation products
clean:
	rm -f *.o *.gch *.d $(ROUTER)

.PHONY: clean debug release

-include Makefile.user
