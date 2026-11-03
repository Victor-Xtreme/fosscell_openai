# Level 2: Debugging Inconsistencies

## Task

Review and fix the inconsistencies and unsafe practices in [`quicksort.c`](quicksort.c).

The solution uses consistent `snake_case` names and formatting, removes an unused header and constant, checks allocation failure, and releases allocated memory before exiting. The quicksort handles empty and one-element ranges through its base case.

## Build and run

Compile with a C compiler that supports C99 or newer:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic quicksort.c -o quicksort
./quicksort
```

The program prints the original and sorted arrays.
