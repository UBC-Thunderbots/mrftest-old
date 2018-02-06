
# folders that have header files we will want to include
LIBRARIES := freertos stm32lib
DIRS = $(LIBRARIES:%=-isystem ../%/include)

# folder to store .o files in. This folder is in the same spot
# as the makefile. Don't delete this folder
DEST = obj

# parse and find folders that contain header files
HEADER_DIRS := main $(filter-out freertos,$(LIBRARIES)) $(if $(filter freertos,$(LIBRARIES)),freertos/include) 
HEADERS := $(shell find $(HEADER_DIRS:%=../%) -maxdepth 1 -type d)

# The last flag includes each of the folders containing headers
# Use ifndef FWTEST endif around code that should be excluded from the
# firmware test compiler. This is typically code that only works on 
# ARM processor.
CFLAGS = -Wall -std=gnu99 -DFWTEST $(HEADERS:%=-I%)

# libraries used
LIBS = -lcheck -lm -lpthread -lrt 

# each variable corresponds to a different folder
# the entries should be a c file you want to compile, but write it with 
# a .o extension
#
# firmware/main
_OBJS = physics.o					
# firmware/main/util
_UTILS = quadratic.o log.o physbot.o
# firmware/main/cvxgen
_CVXGEN = solver.o ldl.o matrix_support.o
# firmware/main/primitives
_PRIMITIVES = move.o primitive.o

# put all of the object files into one variable
_OUTPUT = $(_OBJS) $(_UTILS) $(_CVXGEN) $(_PRIMITIVES)
# adjust the names so that they are outputed into the DEST folder
OUTPUT = $(patsubst %.o, $(DEST)/%.o, $(_OUTPUT))

# entry point for the makefile. runs the test.o rule to compile test.c,
# then compiles everything in $(_OUTPUT) which have their own rules
# based on where the files are
test: test.o $(_OUTPUT)
	@gcc $(CFLAGS) -o $@ $(DEST)/$@.o $(OUTPUT) $(LIBS)
	./test

# for test.c
test.o: 
	@gcc $(CFLAGS) -c $(patsubst %.o, %.c, $@) -o $(DEST)/$@

# an individual set of rules for each folder
# you can see which folder it is in the lines below.
$(_UTILS):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/util/%.c, $@) -o $(DEST)/$@
$(_CVXGEN):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/cvxgen/%.c, $@) -o $(DEST)/$@
$(_OBJS): 
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/%.c, $@) -o $(DEST)/$@
$(_PRIMITIVES):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/primitives/%.c, $@) -o $(DEST)/$@

# can use this rule if you want to echo something
# make view
view:
	@echo $(OUTPUT)

# use this rule to clean the files compiled
# make clean
clean:
	$(RM) $(wildcard $(DEST)/*.o)
	$(RM) test 