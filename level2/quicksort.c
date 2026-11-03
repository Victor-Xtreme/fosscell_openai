
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 10

// Swap two elements
void swap_elements(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Partition the array
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

// Quick sort algorithm
void quick_sort(int arr[], int low, int high) {
    if (arr == NULL || low >= high) {
        return;
    }

    int pivot_index = partition_array(arr, low, high);

    quick_sort(arr, low, pivot_index - 1);
    quick_sort(arr, pivot_index + 1, high);
}

// Print array elements
void print_array(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }

    printf("\n");
}

int main(void) {
    int *data = malloc(ARRAY_SIZE * sizeof(*data));

    // Check memory allocation
    if (data == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return EXIT_FAILURE;
    }

    // Initialize array
    int values[ARRAY_SIZE] = {
        10, 7, 8, 9, 1, 5, 2, 3, 4, 6
    };

    for (int i = 0; i < ARRAY_SIZE; i++) {
        data[i] = values[i];
    }

    printf("Original array:\n");
    print_array(data, ARRAY_SIZE);

    // Sort array
    quick_sort(data, 0, ARRAY_SIZE - 1);

    printf("Sorted array:\n");
    print_array(data, ARRAY_SIZE);

    // Release allocated memory
    free(data);
    data = NULL;

    return EXIT_SUCCESS;
}
