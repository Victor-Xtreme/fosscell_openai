#include <stdio.h>
#include <stdlib.h>

/**
 * Level 2: Debugging Inconsistencies - Fixed Version
 * 
 * Improvements applied:
 * 1. Unified naming convention to snake_case.
 * 2. Handled malloc failure defensively.
 * 3. Deallocated heap memory with free() to prevent leaks.
 * 4. Removed dead imports and unused macro definitions.
 * 5. Normalized code formatting, braces, and array initialization.
 */

/* Swaps the values of two integers */
void swap_elements(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* Partitions the array using the last element as the pivot */
int partition_array(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            swap_elements(&arr[i], &arr[j]);
        }
    }
    swap_elements(&arr[i + 1], &arr[high]);
    return i + 1;
}

/* Sorts an array segment using the QuickSort algorithm */
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pivot_index = partition_array(arr, low, high);
        quick_sort(arr, low, pivot_index - 1);
        quick_sort(arr, pivot_index + 1, high);
    }
}

/* Prints all elements of an integer array */
void print_array(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    const int size = 10;
    int *data = malloc(size * sizeof(int));

    /* Check for memory allocation failure */
    if (data == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    /* Test array elements */
    int sample_values[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    for (int i = 0; i < size; i++) {
        data[i] = sample_values[i];
    }

    printf("Original array:\n");
    print_array(data, size);

    quick_sort(data, 0, size - 1);

    printf("Sorted array:\n");
    print_array(data, size);

    /* Free dynamically allocated memory */
    free(data);
    data = NULL;

    return EXIT_SUCCESS;
}
