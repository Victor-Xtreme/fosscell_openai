#include <stdio.h>
#include <stdlib.h>

/**
 * Level 2: Debugging Inconsistencies
 *
 * This implementation uses consistent naming, formatting, and error handling.
 */

#define ARRAY_SIZE 10

static void swap_elements(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static int partition_array(int arr[], int low, int high) {
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

static void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition_array(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

static void print_array(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    int *data = malloc(ARRAY_SIZE * sizeof(*data));
    if (data == NULL) {
        fprintf(stderr, "Failed to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

    int values[ARRAY_SIZE] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    for (int i = 0; i < ARRAY_SIZE; i++) {
        data[i] = values[i];
    }

    printf("Original array:\n");
    print_array(data, ARRAY_SIZE);

    quick_sort(data, 0, ARRAY_SIZE - 1);

    printf("Sorted array:\n");
    print_array(data, ARRAY_SIZE);

    free(data);
    return EXIT_SUCCESS;
}
