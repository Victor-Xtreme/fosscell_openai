#include <stdio.h>
#include <stdlib.h>

/**
 * Level 2: Debugging Inconsistencies
 * 
 * Your task:
 * 1. Identify and fix code inconsistencies.
 * 2. Address the mixed coding styles (indentation, naming).
 * 3. Find potential memory leaks or undefined behaviors.
 * 4. Ensure error handling is consistent.
 */

void swap_elements(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

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

/* Sort inclusive bounds; low >= high represents a trivial range. */
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pivot_index = partition_array(arr, low, high);
        quick_sort(arr, low, pivot_index - 1);
        quick_sort(arr, pivot_index + 1, high);
    }
}

void print_array(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    const int initial_data[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    const int size = (int)(sizeof initial_data / sizeof initial_data[0]);
    int *data = malloc(sizeof initial_data);

    if (data == NULL) {
        fprintf(stderr, "Error: unable to allocate array memory.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < size; i++) {
        data[i] = initial_data[i];
    }

    printf("Original array: \n");
    print_array(data, size);

    quick_sort(data, 0, size - 1);
    
    printf("Sorted array: \n");
    print_array(data, size);

    free(data);
    return EXIT_SUCCESS;
}
