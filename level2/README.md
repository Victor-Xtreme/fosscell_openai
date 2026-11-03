# Level 2: Debugging Inconsistencies

This level demonstrates a corrected C implementation of quicksort in `quicksort.c`. It sorts a fixed sample of ten integers, prints the values before and after sorting, and reports allocation or output-header failures with a non-zero exit status.

## Build and run

Use a C compiler with C99 or later support:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic -O2 quicksort.c -o quicksort
./quicksort
```

Expected output:

```text
Original array:
10 7 8 9 1 5 2 3 4 6
Sorted array:
1 2 3 4 5 6 7 8 9 10
```

## Corrections made

- Standardized function and variable names to `snake_case` and used consistent brace/indentation style.
- Removed unused `<time.h>` and the unrelated `MAX_SIZE` constant.
- Used `size_t` indices and half-open ranges `[low, high)` to avoid signed index arithmetic and boundary underflow in partitioning.
- Checked the result of `malloc` before writing through the pointer.
- Freed the allocated array on normal completion and on the handled output-header error paths.
- Used `sizeof(*data)` so the allocation size stays tied to the pointed-to type.
- Added explicit return statuses through `EXIT_SUCCESS` and `EXIT_FAILURE`.

The sample is intentionally small. Lomuto quicksort can still take quadratic time and deep recursion on adversarially ordered data; this demonstration is not intended as a hardened general-purpose sorting library.
