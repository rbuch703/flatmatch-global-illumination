
SRC = main.cc png_helper.cc parseLayout.cc vector3.cc

OBJ  = $(SRC:.cc=.o)

# -ftrapv is extremely important to catch integer overflow bugs, which are otherwise hard to detect
# OSM data covers almost the entire range of int32_t; multiplying two values (required for some algorithms)
# already uses all bits of an int64_t, so, more complex algorithms could easily cause an - otherwise undetected -
# integer overflow
# WARNING: the gcc option -O2 appears to negate the effects of -ftrapv ! 

FLAGS = -g -Wall -Wextra -DNDEBUG -O2

#FLAGS = -ftrapv -g -Wall -Wextra 
#FLAGS = -ftrapv -g -Wall -Wextra -fprofile-arcs -ftest-coverage
CFLAGS = $(FLAGS) -std=c99
CCFLAGS = $(FLAGS) -std=c++11
LD_FLAGS = #-fprofile-arcs#--as-needed
.PHONY: all clean

all: make.dep radiosity
#	 @echo [ALL] $<

radiosity: $(OBJ)
	@echo [LD ] $@
	@g++ $^  $(LD_FLAGS) -lpng -o $@

%.o: %.cc
	@echo [C++] $<
#	@g++ $(CCFLAGS) `pkg-config --cflags cairo` $< -c -o $@
	@g++ $(CCFLAGS) $< -c -o $@

	
clean:
	@echo [CLEAN]
	@rm -rf *.o
	@rm -rf *~
	@rm -rf radiosity
	@rm -rf coverage.info callgrind.out.*

make.dep: $(SRC)
	@echo [DEP]
	@g++ -MM $^ > make.dep

include make.dep

