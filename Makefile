
SRC = png_helper.c main.cc parseLayout.cc sceneObject.c vector3_sse.c #vector3.cc

#OBJ  = $(SRC:.cc=.o)
OBJ = png_helper.o main.o parseLayout.o sceneObject.o vector3_sse.o

OPT_FLAGS = -O2 
FLAGS = -g -Wall -Wextra -msse3 -DNDEBUG $(OPT_FLAGS) -flto

PROFILE =
#PROFILE = -fprofile-generate
#PROFILE = -fprofile-use

CFLAGS = $(FLAGS) $(PROFILE) -std=c99
CCFLAGS = $(FLAGS) $(PROFILE) -std=c++11
LD_FLAGS = $(PROFILE) -lm #-flto $(OPT_FLAGS) 
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

sceneObject.o: sceneObject.c
	@echo [CC] $<
#	@g++ $(CCFLAGS) `pkg-config --cflags cairo` $< -c -o $@
	@gcc $(CFLAGS) $< -c -o $@

png_helper.o: png_helper.c
	@echo [CC] $<
#	@g++ $(CCFLAGS) `pkg-config --cflags cairo` $< -c -o $@
	@gcc $(CFLAGS) $< -c -o $@

vector3_sse.o: vector3_sse.c
	@echo [CC] $<
#	@g++ $(CCFLAGS) `pkg-config --cflags cairo` $< -c -o $@
	@gcc $(CFLAGS) $< -c -o $@

	
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

