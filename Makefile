
SRC_C = png_helper.c rectangle.c vector3_cl.c
SRC_CC = main.cc parseLayout.cc
SRC = $(SRC_C) $(SRC_CC)

OBJ_C  = $(SRC_C:.c=.o)
OBJ_CC = $(SRC_CC:.cc=.oo)
OBJ    = $(OBJ_C) $(OBJ_CC)

OPT_FLAGS = #-O2
OSX_INCLUDES = #-I /usr/local/include -framework OpenCL
OSX_LIBS = #-L /usr/local/lib -framework OpenCL
FLAGS = -g -Wall -Wextra -msse3 $(OPT_FLAGS)
PROFILE = 
#PROFILE = -fprofile-generate
#PROFILE = -fprofile-use

CFLAGS = $(FLAGS) $(PROFILE) -std=c99 #$(OSX_INCLUDES)
CCFLAGS = $(FLAGS) $(PROFILE) -std=c++11 #$(OSX_INCLUDES)
LD_FLAGS = $(PROFILE) $(OSX_LIBS) -lOpenCL -lm #-flto $(OPT_FLAGS) 
.PHONY: all clean

all: make.dep globalIllumination tiles
#	 @echo [ALL] $<

globalIllumination: $(OBJ_C) $(OBJ_CC)
	@echo [LD] $@
	@g++ $^  $(LD_FLAGS) -lpng -o $@

%.o: %.c
	@echo [CC] $<
	@gcc $(CFLAGS) $< -c -o $@

tiles:
	@echo [MKDIR] tiles
	@mkdir tiles
    
%.oo: %.cc
	@echo [CP] $<
	@g++ $(CCFLAGS) $< -c -o $@

clean:
	@echo [CLEAN]
	@rm -rf *.o *.oo
	@rm -rf *~
	@rm -rf globalIllumination
	@rm -rf coverage.info callgrind.out.*

make.dep: $(SRC)
	@echo [DEP]
	@g++ -MM -I /usr/local/include $^ > make.dep

include make.dep

