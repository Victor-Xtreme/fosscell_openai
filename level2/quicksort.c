
#include <stdio.h>
#include <stdlib.h>

#define DATA_SIZE 10

// Swap two elements
static void swap_elements(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Partition the array around the pivot
static int partition_array(int arr[], int low, int high) {
    int pivot = arr[high];
    int i = low;

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            swap_elements(&arr[i], &arr[j]);
            i++;
        }
    }

    swap_elements(&arr[i], &arr[high]);
    return i;
}

// Quick Sort implementation
static void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pivot_index = partition_array(arr, low, high);

        quick_sort(arr, low, pivot_index - 1);
        quick_sort(arr, pivot_index + 1, high);
    }
}

// Print array elements
static void print_array(const int arr[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    const int initial_data[DATA_SIZE] = {
        10, 7, 8, 9, 1, 5, 2, 3, 4, 6
    };

    int *data = malloc(DATA_SIZE * sizeof(*data));

    // Check memory allocation
    if (data == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    // Copy test data
    for (size_t i = 0; i < DATA_SIZE; i++) {
        data[i] = initial_data[i];
    }

    printf("Original array:\n");
    print_array(data, DATA_SIZE);

    quick_sort(data, 0, DATA_SIZE - 1);

    printf("Sorted array:\n");
    print_array(data, DATA_SIZE);

    // Release allocated memory
    free(data);
    data = NULL;

    return EXIT_SUCCESS;
}
