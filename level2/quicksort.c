#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 10

static void swap_elements(int *left, int *right)
{
    int temporary = *left;
    *left = *right;
    *right = temporary;
}

static int partition_array(int array[], int low, int high)
{
    int pivot_value = array[high];
    int partition_index = low - 1;

    for (int current_index = low; current_index < high; current_index++) {
        if (array[current_index] < pivot_value) {
            partition_index++;
            swap_elements(&array[partition_index], &array[current_index]);
        }
    }

    swap_elements(&array[partition_index + 1], &array[high]);
    return partition_index + 1;
}

static void quick_sort(int array[], int low, int high)
{
    if (low < high) {
        int pivot_index = partition_array(array, low, high);
        quick_sort(array, low, pivot_index - 1);
        quick_sort(array, pivot_index + 1, high);
    }
}

static void print_array(const int array[], int size)
{
    for (int index = 0; index < size; index++) {
        printf("%d ", array[index]);
    }
    printf("\n");
}

int main(void)
{
    const int initial_values[ARRAY_SIZE] = {
        10, 7, 8, 9, 1, 5, 2, 3, 4, 6
    };
    int *data = malloc(ARRAY_SIZE * sizeof(*data));

    if (data == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

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
