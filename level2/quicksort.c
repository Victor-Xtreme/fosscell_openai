#include <stdio.h>
#include <stdlib.h>

/**
 * Level 2: Debugging Inconsistencies
 *
 * Use consistent naming and formatting, check allocation failures,
 * and release allocated memory.
 */

static void swap_elements(int *a, int *b) {
    int temporary = *a;
    *a = *b;
    *b = temporary;
}

static int partition_array(int array[], int low, int high) {
    int pivot = array[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (array[j] < pivot) {
            i++;
            swap_elements(&array[i], &array[j]);
        }
    }
    swap_elements(&array[i + 1], &array[high]);
    return i + 1;
}

static void quick_sort(int array[], int low, int high) {
    if (low < high) {
        int pivot_index = partition_array(array, low, high);
        quick_sort(array, low, pivot_index - 1);
        quick_sort(array, pivot_index + 1, high);
    }
}

static void print_array(const int array[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

int main(void) {
    const int initial_data[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    const int size = (int)(sizeof initial_data / sizeof initial_data[0]);
    int *data = malloc(sizeof initial_data);

    if (data == NULL) {
        fprintf(stderr, "Failed to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < size; i++) {
        data[i] = initial_data[i];
    }

    printf("Original array:\n");
    print_array(data, size);

    quick_sort(data, 0, size - 1);

    printf("Sorted array:\n");
    print_array(data, size);

    free(data);
    return EXIT_SUCCESS;
}
