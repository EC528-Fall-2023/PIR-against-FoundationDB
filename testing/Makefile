GCC=gcc
FLAGS=-std=c11 -g -lm -lpthread -lrt

all: src/test.c
	$(GCC) $(FLAGS) src/test.c /usr/lib64/libfdb_c.so -o test

clean:
	rm test
