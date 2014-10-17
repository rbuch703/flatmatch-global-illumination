
SRC_C = png_helper.c rectangle.c vector3_cl.c photonmap.c
SRC_CC = main.cc parseLayout.cc global_illumination_cl.cc
SRC = $(SRC_C) $(SRC_CC)

OBJ_C  = $(SRC_C:.c=.o)
OBJ_CC = $(SRC_CC:.cc=.oo)
OBJ    = $(OBJ_C) $(OBJ_CC)

CC = gcc#clang
CPP= g++#clang++

OPT_FLAGS = #-O2
OSX_INCLUDES = #-I /usr/local/include -framework OpenCL
OSX_LIBS = #-L /usr/local/lib -framework OpenCL
FLAGS = -g -Wall -Wextra -msse3 $(OPT_FLAGS)
PROFILE = 
#PROFILE = -fprofile-generate
#PROFILE = -fprofile-use

CFLAGS = $(FLAGS) $(PROFILE) -std=c99 -flto #$(OSX_INCLUDES)
CCFLAGS = $(FLAGS) $(PROFILE) -std=c++11 -flto #$(OSX_INCLUDES)
LD_FLAGS = $(PROFILE) $(OSX_LIBS) -lOpenCL -lm  $(OPT_FLAGS)  #-flto
.PHONY: all clean

all: make.dep globalIllumination tiles
#	 @echo [ALL] $<

globalIllumination: $(OBJ_C) $(OBJ_CC)
	@echo [LD] $@
	@$(CPP) $^  $(LD_FLAGS) -lpng -o $@

%.o: %.c
	@echo [CC] $<
	@$(CC) $(CFLAGS) $< -c -o $@

tiles:
	@echo [MKDIR] tiles
	@mkdir tiles
    
%.oo: %.cc
	@echo [CP] $<
	@$(CPP) $(CCFLAGS) $< -c -o $@

clean:
	@echo [CLEAN]
	@rm -rf *.o *.oo
	@rm -rf *~
	@rm -rf globalIllumination
	@rm -rf coverage.info callgrind.out.*

make.dep: $(SRC_C) $(SRC_CC)
	@echo [DEP]
	@$(CC)  -MM -I /usr/local/include $(SRC_C) > make.dep
	@$(CPP) -MM -I /usr/local/include $(SRC_CC) >> make.dep

include make.dep

