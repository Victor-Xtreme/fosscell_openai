#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Level 2: Debugging Inconsistencies
 * 
 * Your task:
 * 1. Identify and fix code inconsistencies.
 * 2. Address the mixed coding styles (indentation, naming).
 * 3. Find potential memory leaks or undefined behaviors.
 * 4. Ensure error handling is consistent.
 *
 * ---------------------------------------------------------------------
 * CHANGE LOG / DOCUMENTATION
 * ---------------------------------------------------------------------
 * Naming
 *   - All functions renamed to snake_case (swap_elements, quick_sort,
 *     print_array; partition_array already conformed).
 *   - Helpers are `static` (internal linkage, no namespace pollution).
 *
 * Memory / undefined behavior  
 *   - malloc() result is checked; failure is reported and handled.
 *   - Allocated memory is always freed (single cleanup path in main).
 *   - Removed the unneeded (int *) cast on malloc (C, not C++).
 *   - sizeof *data used instead of sizeof(int) so it follows the type.
 *   - Recursion never computes pi - 1 / pi + 1 without a range guard,
 *     so the code stays correct if indices become unsigned (size_t).
 *
 * Algorithm robustness
 *   - Pivot is chosen randomly (srand/rand, which uses <time.h>)
 *     instead of always arr[high]. This avoids the O(n^2) worst case on
 *     already-sorted / reverse-sorted input.
 *   - quick_sort recurses only into the SMALLER partition and loops on
 *     the larger one, bounding stack depth to O(log n) and preventing
 *     stack overflow on adversarial input.
 *
 * Constants
 *   - MAX_SIZE is now used: an upper bound validated at compile time
 *     (_Static_assert) and at runtime (sort_array).
 *   - The hardcoded 10 is replaced by ARRAY_SIZE.
 *
 * Error handling (consistent convention)
 *   - Functions that can fail return STATUS_OK (0) or STATUS_ERROR (-1).
 *   - Errors are written to stderr with an "Error:" prefix.
 *   - main() returns EXIT_SUCCESS / EXIT_FAILURE accordingly.
 *
 * Style
 *   - Braces on every block (print_array previously had none).
 *   - Redundant parentheses removed; main(void); const-correct params;
 *     one statement per line; initializer list replaces data[0..9].
 * ---------------------------------------------------------------------
 */

// Global constant
#define MAX_SIZE 1000

/* Test data size (must not exceed MAX_SIZE). */
#define ARRAY_SIZE 10

/* Status codes used by every function that can fail. */
#define STATUS_OK     0
#define STATUS_ERROR (-1)

_Static_assert(ARRAY_SIZE <= MAX_SIZE, "ARRAY_SIZE must not exceed MAX_SIZE");

// Inconsistent naming: CamelCase and snake_case
// (Resolved: everything is now snake_case, e.g. SwapElements -> swap_elements.)
static void swap_elements(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

/*
 * Lomuto partition with a randomly chosen pivot.
 * The random pivot is swapped into arr[high] first, so the original
 * partition logic below is unchanged.
 * Precondition: low < high.
 */
static int partition_array(int arr[], int low, int high)
{
    int rand_index = low + rand() % (high - low + 1);
    swap_elements(&arr[rand_index], &arr[high]);

    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap_elements(&arr[i], &arr[j]);
        }
    }
    swap_elements(&arr[i + 1], &arr[high]);
    return i + 1;
}

// Inconsistent function style
// (Resolved: same brace/naming style as the rest of the file; recursion
// depth is bounded by recursing into the smaller half only.)
static void quick_sort(int arr[], int low, int high)
{
    while (low < high) {
        int pi = partition_array(arr, low, high);

        if (pi - low < high - pi) {
            if (pi > low) {
                quick_sort(arr, low, pi - 1);
            }
            low = pi + 1;
        } else {
            quick_sort(arr, pi + 1, high);
            if (pi == low) {
                break;
            }
            high = pi - 1;
        }
    }
}

/* Validated entry point: returns STATUS_OK or STATUS_ERROR. */
static int sort_array(int arr[], int size)
{
    if (arr == NULL) {
        fprintf(stderr, "Error: sort_array received a NULL array\n");
        return STATUS_ERROR;
    }
    if (size < 0 || size > MAX_SIZE) {
        fprintf(stderr, "Error: size %d is outside the range 0..%d\n",
                size, MAX_SIZE);
        return STATUS_ERROR;
    }
    if (size > 1) {
        quick_sort(arr, 0, size - 1);
    }
    return STATUS_OK;
}

static void print_array(const int arr[], int size)
{
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void)
{
    int status = EXIT_SUCCESS;

    srand((unsigned int)time(NULL));

    int *data = malloc(ARRAY_SIZE * sizeof *data);
    if (data == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return EXIT_FAILURE;
    }
    
    // Hardcoded test
    // (Now an initializer list instead of ten separate assignments.)
    static const int test_values[ARRAY_SIZE] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    for (int i = 0; i < ARRAY_SIZE; i++) {
        data[i] = test_values[i];
    }

    printf("Original array: \n");
    print_array(data, ARRAY_SIZE);

    if (sort_array(data, ARRAY_SIZE) != STATUS_OK) {
        status = EXIT_FAILURE;
    } else {
        printf("Sorted array: \n");
        print_array(data, ARRAY_SIZE);
    }

    // Missing free!
    // (Fixed: freed on every path after a successful allocation.)
    free(data);
    return status;
}