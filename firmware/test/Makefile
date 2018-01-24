


LIBRARIES := freertos stm32lib
DIRS = $(LIBRARIES:%=-isystem ../%/include)
DEST = obj

HEADER_DIRS := main $(filter-out freertos,$(LIBRARIES)) $(if $(filter freertos,$(LIBRARIES)),freertos/include) 
HEADERS := $(shell find $(HEADER_DIRS:%=../%) -maxdepth 1 -type d)


CFLAGS = -Wall -std=gnu99 -DFWTEST $(HEADERS:%=-I%)
LIBS = -lcheck -lm -lpthread -lrt 

LINKER_SCRIPT := ../stm32lib/stm32f405.ld

_OBJS = physics.o
_UTILS = quadratic.o log.o physbot.o
_CVXGEN = solver.o ldl.o matrix_support.o
_PRIMITIVES = move.o primitive.o

_OUTPUT = $(_OBJS) $(_UTILS) $(_CVXGEN) $(_PRIMITIVES)
OUTPUT = $(patsubst %.o, obj/%.o, $(_OUTPUT))


test: test.o $(_OUTPUT) $(LINKER_SCRIPT)
	@gcc $(CFLAGS) -o $@ $(DEST)/$@.o $(OUTPUT) $(LIBS)
	./test

test.o: 
	@gcc $(CFLAGS) -c $(patsubst %.o, %.c, $@) -o $(DEST)/$@

$(_UTILS):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/util/%.c, $@) -o $(DEST)/$@

$(_CVXGEN):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/cvxgen/%.c, $@) -o $(DEST)/$@

$(_OBJS): 
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/%.c, $@) -o $(DEST)/$@

$(_PRIMITIVES):
	@gcc $(CFLAGS) -c $(patsubst %.o, ../main/primitives/%.c, $@) -o $(DEST)/$@

view:
	@echo $(OUTPUT)

clean:
	$(RM) $(wildcard $(DEST)/*.o)
	$(RM) test 