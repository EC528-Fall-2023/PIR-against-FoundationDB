GCC=gcc
FLAGS=-std=c11 -g

all: src/test.c
	$(GCC) $(FLAGS) src/test.c /usr/local/lib/libfdb_c.dylib -o test.exe

clean:
	rm test.exe
