# Level 2: Debugging Inconsistencies

Your task is to identify and fix code inconsistencies in `data_processor.py`. 
These are not syntactic errors; the code will run, but it follows poor practices or violates internal consistency.

Look for:
- Inconsistent variable naming (e.g., snake_case vs camelCase).
- Inconsistent error handling patterns.
- Repeated logic that could be refactored.

CHANGE LOG / DOCUMENTATION

Naming
  - All functions renamed to snake_case (swap_elements, quick_sort,
    print_array; partition_array already conformed).
  - Helpers are static (internal linkage, no namespace pollution).

Memory / undefined behavior
  - malloc() result is checked; failure is reported and handled.
  - Allocated memory is always freed (single cleanup path in main).
  - Removed the unneeded (int *) cast on malloc (C, not C++).
  - sizeof *data used instead of sizeof(int) so it follows the type.
  - Recursion never computes pi - 1 / pi + 1 without a range guard,
    so the code stays correct if indices become unsigned (size_t).

Algorithm robustness
  - Pivot is chosen randomly (srand/rand, which uses <time.h>)
    instead of always arr[high]. This avoids the O(n^2) worst case on
    already-sorted / reverse-sorted input.
  - quick_sort recurses only into the SMALLER partition and loops on
    the larger one, bounding stack depth to O(log n) and preventing
    stack overflow on adversarial input.

Constants
  - MAX_SIZE is now used: an upper bound validated at compile time
    (_Static_assert) and at runtime (sort_array).
  - The hardcoded 10 is replaced by ARRAY_SIZE.

Error handling (consistent convention)
  - Functions that can fail return STATUS_OK (0) or STATUS_ERROR (-1).
  - Errors are written to stderr with an "Error:" prefix.
  - main() returns EXIT_SUCCESS / EXIT_FAILURE accordingly.
  - New sort_array() wrapper validates NULL and size before sorting.

Style
  - Braces on every block (print_array previously had none).
  - Redundant parentheses removed; main(void); const-correct params;
    one statement per line; initializer list replaces data[0..9].
