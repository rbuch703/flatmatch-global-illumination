
SRC = vector3.cc png_helper.cc main.cc parseLayout.cc sceneObject.cc

OBJ  = $(SRC:.cc=.o)

OPT_FLAGS = -O2
FLAGS = -g -Wall -Wextra -DNDEBUG $(OPT_FLAGS) -flto

#FLAGS = -ftrapv -g -Wall -Wextra 
#FLAGS = -ftrapv -g -Wall -Wextra -fprofile-arcs -ftest-coverage
CFLAGS = $(FLAGS) -std=c99
CCFLAGS = $(FLAGS) -std=c++11
LD_FLAGS = -flto $(OPT_FLAGS) #-fprofile-arcs#--as-needed
.PHONY: all clean

all: make.dep radiosity
#	 @echo [ALL] $<

radiosity: $(OBJ)
	@echo [LD ] $@
	@g++ $^  $(LD_FLAGS) -lpng -o $@

#fast: $(SRC)
#	@echo [C++] $(SRC)
#	@g++ $(CCFLAGS) -fwhole-program -o radiosity $(SRC) -lpng
#	#@g++ $(CCFLAGS) -o radiosity $(SRC) -lpng
	
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

