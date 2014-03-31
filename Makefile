
SRC = png_helper.cc main.cc parseLayout.cc sceneObject.cc #vector3.cc

OBJ  = $(SRC:.cc=.o)

OPT_FLAGS = -O2 
FLAGS = -g -Wall -Wextra -msse3 -DNDEBUG $(OPT_FLAGS)  #-flto

PROFILE =
#PROFILE = -fprofile-generate
#PROFILE = -fprofile-use

CFLAGS = $(FLAGS) $(PROFILE) -std=c99
CCFLAGS = $(FLAGS) $(PROFILE) -std=c++11
LD_FLAGS = $(PROFILE) #-flto $(OPT_FLAGS) 
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

