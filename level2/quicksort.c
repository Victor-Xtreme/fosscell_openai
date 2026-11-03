#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Level 2: Debugging Inconsistencies (fixed)
 *
 * Consistency fixes:
 * - All functions use snake_case naming.
 * - Every function that can fail returns a status code checked by the caller.
 * - Array size is derived once instead of hardcoded in several places.
 * - Allocated memory is always freed.
 * - Sizes and indices are size_t throughout (no int/size_t mixing).
 *
 * Algorithm: introsort, the scheme used by C++ std::sort.
 * - Median-of-three pivot: sorted and reversed input no longer hit O(n^2).
 * - Three-way partition (< pivot, == pivot, > pivot): many duplicate values
 *   sort in linear time instead of degrading to O(n^2).
 * - Recurse into the smaller side, loop over the larger: O(log n) stack depth.
 * - Heapsort fallback when recursion gets too deep: guaranteed O(n log n),
 *   even on inputs crafted to defeat median-of-three.
 * - Insertion sort for small ranges, where it beats quicksort in practice.
 * - O(n) pre-pass: already sorted input returns at once, descending is reversed.
 *
 * Build and run: make        (demo)
 *                make test   (correctness tests, see tests/test_quicksort.c)
 *                make bench  (timing against the C library's qsort)
 */

/* Ranges at or below this size are finished with insertion sort. */
#define INSERTION_SORT_THRESHOLD 16

static void swap_elements(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static void insertion_sort(int arr[], size_t size) {
    for (size_t i = 1; i < size; i++) {
        int key = arr[i];
        size_t j = i;
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

/* Restores the max-heap property for the subtree rooted at `root`. */
static void sift_down(int arr[], size_t root, size_t size) {
    for (;;) {
        size_t child = 2 * root + 1;
        if (child >= size) {
            return;
        }
        if (child + 1 < size && arr[child] < arr[child + 1]) {
            child++;
        }
        if (arr[root] >= arr[child]) {
            return;
        }
        swap_elements(&arr[root], &arr[child]);
        root = child;
    }
}

static void heap_sort(int arr[], size_t size) {
    for (size_t i = size / 2; i-- > 0;) {
        sift_down(arr, i, size);
    }
    for (size_t end = size; end-- > 1;) {
        swap_elements(&arr[0], &arr[end]);
        sift_down(arr, 0, end);
    }
}

static int median_of_three(int a, int b, int c) {
    if (a < b) {
        if (b < c) {
            return b;
        }
        return (a < c) ? c : a;
    }
    if (a < c) {
        return a;
    }
    return (b < c) ? c : b;
}

/*
 * Rearranges arr into three regions: [0, *less_end) < pivot,
 * [*less_end, *greater_start) == pivot and [*greater_start, size) > pivot.
 */
static void partition_three_way(int arr[], size_t size, int pivot,
                                size_t *less_end, size_t *greater_start) {
    size_t lt = 0;
    size_t i = 0;
    size_t gt = size;

    while (i < gt) {
        if (arr[i] < pivot) {
            swap_elements(&arr[lt++], &arr[i++]);
        } else if (arr[i] > pivot) {
            swap_elements(&arr[i], &arr[--gt]);
        } else {
            i++;
        }
    }
    *less_end = lt;
    *greater_start = gt;
}

static void introsort_range(int arr[], size_t size, unsigned depth_limit) {
    while (size > INSERTION_SORT_THRESHOLD) {
        if (depth_limit == 0) {
            heap_sort(arr, size);
            return;
        }
        depth_limit--;

        int pivot = median_of_three(arr[0], arr[size / 2], arr[size - 1]);
        size_t less_end;
        size_t greater_start;
        partition_three_way(arr, size, pivot, &less_end, &greater_start);

        size_t left_size = less_end;
        size_t right_size = size - greater_start;

        /* Recurse into the smaller side and loop over the larger one. */
        if (left_size < right_size) {
            introsort_range(arr, left_size, depth_limit);
            arr += greater_start;
            size = right_size;
        } else {
            introsort_range(arr + greater_start, right_size, depth_limit);
            size = left_size;
        }
    }
    insertion_sort(arr, size);
}

/* 2 * floor(log2(size)): the standard introsort recursion budget. */
static unsigned depth_limit_for(size_t size) {
    unsigned log2_size = 0;
    while (size > 1) {
        size >>= 1;
        log2_size++;
    }
    return 2 * log2_size;
}

/*
 * O(n) pre-pass for the most common real-world orderings: input that is
 * already ascending is left alone, and descending input is reversed.
 * Returns 1 if the array is now sorted.
 */
static int handle_presorted(int arr[], size_t size) {
    size_t i = 1;
    while (i < size && arr[i - 1] <= arr[i]) {
        i++;
    }
    if (i == size) {
        return 1;
    }

    i = 1;
    while (i < size && arr[i - 1] >= arr[i]) {
        i++;
    }
    if (i == size) {
        for (size_t lo = 0, hi = size - 1; lo < hi; lo++, hi--) {
            swap_elements(&arr[lo], &arr[hi]);
        }
        return 1;
    }
    return 0;
}

/* Public entry point: sorts the whole array. Returns 0 on success, -1 on bad input. */
static int quick_sort(int arr[], size_t size) {
    if (arr == NULL && size > 0) {
        return -1;
    }
    if (size > 1 && !handle_presorted(arr, size)) {
        introsort_range(arr, size, depth_limit_for(size));
    }
    return 0;
}

/* Tests and benchmarks #include this file and define QUICKSORT_NO_MAIN. */
#ifndef QUICKSORT_NO_MAIN
static void print_array(const char *label, const int arr[], size_t size) {
    printf("%s\n", label);
    for (size_t i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    const int initial_values[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    const size_t size = sizeof(initial_values) / sizeof(initial_values[0]);

    int *data = malloc(size * sizeof(*data));
    if (data == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return EXIT_FAILURE;
    }
    memcpy(data, initial_values, size * sizeof(*data));

    print_array("Original array:", data, size);

    if (quick_sort(data, size) != 0) {
        fprintf(stderr, "Error: invalid input to quick_sort\n");
        free(data);
        return EXIT_FAILURE;
    }

    print_array("Sorted array:", data, size);

    free(data);
    return EXIT_SUCCESS;
}
#endif
