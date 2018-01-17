CFLAGS=-O0 -g

all: crepl tests

crepl: crepl.o evaluator.o

tests: tests.o evaluator.o

clean:
	-rm *.o
	-rm crepl
	-rm tests

.PHONY=all

