#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 10

static void swap_elements(int *left, int *right) {
    int temporary = *left;
    *left = *right;
    *right = temporary;
}

static int partition_array(int values[], int low, int high) {
    int pivot = values[high];
    int boundary = low;

    for (int index = low; index < high; index++) {
        if (values[index] < pivot) {
            swap_elements(&values[boundary], &values[index]);
            boundary++;
        }
    }

    swap_elements(&values[boundary], &values[high]);
    return boundary;
}

static void quick_sort(int values[], int low, int high) {
    if (low >= high) {
        return;
    }

    int pivot_index = partition_array(values, low, high);
    quick_sort(values, low, pivot_index - 1);
    quick_sort(values, pivot_index + 1, high);
}

static void print_array(const int values[], int size) {
    for (int index = 0; index < size; index++) {
        printf("%d%s", values[index], index + 1 == size ? "\n" : " ");
    }
}

int main(void) {
    int *data = malloc(ARRAY_SIZE * sizeof(*data));
    if (data == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

    const int initial_values[ARRAY_SIZE] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    for (int index = 0; index < ARRAY_SIZE; index++) {
        data[index] = initial_values[index];
    }

    printf("Original array:\n");
    print_array(data, ARRAY_SIZE);

    quick_sort(data, 0, ARRAY_SIZE - 1);

    printf("Sorted array:\n");
    print_array(data, ARRAY_SIZE);

    free(data);
    return EXIT_SUCCESS;
}
