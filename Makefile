# Taken largely from http://scottmcpeak.com/autodepend/autodepend.html

CXXFLAGS := -std=c++11 -Wall -Wextra -pedantic
LIBFLAGS :=

OBJS := $(patsubst %.cpp,%.o, $(wildcard *.cpp))

#OPTIMIZATIONS := -O2 -DNDEBUG -flto
OPTIMIZATIONS := -O2 -flto

debug: CXXFLAGS += -g
debug: route

profile: CXXFLAGS += $(OPTIMIZATIONS) -g -pg
profile: route

release: CXXFLAGS+= $(OPTIMIZATIONS)
release: route

# link
route: $(OBJS) main.o
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBFLAGS) -o route

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
	rm -f *.o *.gch *.d route

.PHONY: clean debug release
