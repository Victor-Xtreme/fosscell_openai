LEVEL 2

#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 1000

// Swap two array elements
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

// Sort the array using QuickSort
void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pivot_index = partition_array(arr, low, high);

        quick_sort(arr, low, pivot_index - 1);
        quick_sort(arr, pivot_index + 1, high);
    }
}

// Print array elements
void print_array(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }

    printf("\n");
}

int main(void) {
    int size = 10;

    // Allocate memory
    int *data = malloc(size * sizeof(*data));

    // Check memory allocation
    if (data == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    // Initialize array
    int initial_data[] = {
        10, 7, 8, 9, 1,
        5, 2, 3, 4, 6
    };

    for (int i = 0; i < size; i++) {
        data[i] = initial_data[i];
    }

    printf("Original array:\n");
    print_array(data, size);

    // Sort array
    quick_sort(data, 0, size - 1);

    printf("Sorted array:\n");
    print_array(data, size);

    // Release allocated memory
    free(data);
    data = NULL;

    return EXIT_SUCCESS;
}
2. Problems identified and fixed Original issue
Correction

SwapElements(),
	changed to swap_elements()

QuickSort(),
	Changed to quick_sort()

printArray(),
	Changed to print_array()

Mixed naming styles,
	Standardized to snake_case

No memory allocation check,
	Added data == NULL check

Missing free(),
	Added free(data)

Hardcoded array size,
	Introduced size variable

Inconsistent indentation,
	Standardized indentation

Repeated array initialization,
	Used an initialization array

Unclear variable pi,
	Changed to pivot_index

Missing const,
	Added const to the print function
  
  
3. Important memory and safety improvements
1. Memory allocation check
int *data = malloc(size * sizeof(*data));

if (data == NULL) {
    fprintf(stderr, "Error: Memory allocation failed.\n");
    return EXIT_FAILURE;
}

Prevents accessing memory if allocation fails.

2. Memory leak prevention
free(data);
data = NULL;
Releases allocated memory after sorting.

3. Consistent function naming

All functions now follow snake_case:
- swap_elements()
- partition_array()
- quick_sort()
- print_array()
4. Expected output
Original array:
10 7 8 9 1 5 2 3 4 6

Sorted array:
1 2 3 4 5 6 7 8 9 10

