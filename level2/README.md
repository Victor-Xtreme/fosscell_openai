# Level 2: Debugging Inconsistencies

## Task

The original Level 2 README refers to `data_processor.py`, which is under
`level1/`; `level2/quicksort.c` also contains a Level 2 task in its comments.
Both provided code paths are addressed: the Python program now shares one
latency-group accumulator and restores garbage collection to its prior state,
while the C program uses consistent `snake_case` names, handles allocation
failure, frees memory, and limits quicksort call-stack use.

The C program's Lomuto partition scheme still has worst-case running time
O(n²), but recursing into the smaller partition and looping over the larger
one keeps call-stack use logarithmic even when partitions are highly unbalanced.

## Build and run

Compile with a C compiler that supports C99 or newer:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic quicksort.c -o quicksort
./quicksort
```

The program prints the original array followed by the sorted values `1 2 3 4 5 6 7 8 9 10`.
