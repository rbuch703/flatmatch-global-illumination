
SRC_C = png_helper.c rectangle.c vector3_cl.c
SRC_CC = main.cc parseLayout.cc
SRC = $(SRC_C) $(SRC_CC)

OBJ_C  = $(SRC_C:.c=.o)
OBJ_CC = $(SRC_CC:.cc=.oo)
OBJ    = $(OBJ_C) $(OBJ_CC)

OPT_FLAGS = -O2 
FLAGS = -g -Wall -Wextra -msse3 -DNDEBUG $(OPT_FLAGS) -flto

PROFILE =
#PROFILE = -fprofile-generate
#PROFILE = -fprofile-use

CFLAGS = $(FLAGS) $(PROFILE) -std=c99
CCFLAGS = $(FLAGS) $(PROFILE) -std=c++11
LD_FLAGS = $(PROFILE) -lm -lOpenCL -flto $(OPT_FLAGS) 
.PHONY: all clean

all: make.dep radiosity
#	 @echo [ALL] $<

radiosity: $(OBJ_C) $(OBJ_CC)
	@echo [LD] $@
	@g++ $^  $(LD_FLAGS) -lpng -o $@

%.o: %.c
	@echo [CC] $<
	@gcc $(CFLAGS) $< -c -o $@

	
%.oo: %.cc
	@echo [CP] $<
	@g++ $(CCFLAGS) $< -c -o $@

clean:
	@echo [CLEAN]
	@rm -rf *.o *.oo
	@rm -rf *~
	@rm -rf radiosity
	@rm -rf coverage.info callgrind.out.*

make.dep: $(SRC)
	@echo [DEP]
	@g++ -MM $^ > make.dep

include make.dep

