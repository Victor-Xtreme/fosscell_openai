CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2

.PHONY: all run test bench clean

all: quicksort

quicksort: quicksort.c
	$(CC) $(CFLAGS) -o $@ quicksort.c

run: quicksort
	./quicksort

test_quicksort: tests/test_quicksort.c quicksort.c
	$(CC) $(CFLAGS) -o $@ tests/test_quicksort.c

test: test_quicksort
	./test_quicksort

bench_quicksort: tests/bench_quicksort.c quicksort.c
	$(CC) $(CFLAGS) -O3 -o $@ tests/bench_quicksort.c

bench: bench_quicksort
	./bench_quicksort

clean:
	rm -f quicksort test_quicksort bench_quicksort *.exe
