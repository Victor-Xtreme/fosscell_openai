#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 10

static void swap_elements(int *left, int *right)
{
    int temporary = *left;
    *left = *right;
    *right = temporary;
}

/* Partition arr[low, high) and return the pivot's final index. */
static size_t partition_array(int arr[], size_t low, size_t high)
{
    int pivot = arr[high - 1];
    size_t smaller_end = low;

    for (size_t current = low; current < high - 1; ++current) {
        if (arr[current] < pivot) {
            swap_elements(&arr[smaller_end], &arr[current]);
            ++smaller_end;
        }
    }

    swap_elements(&arr[smaller_end], &arr[high - 1]);
    return smaller_end;
}

/* Sort the half-open range arr[low, high). */
static void quick_sort(int arr[], size_t low, size_t high)
{
    if (high - low < 2) {
        return;
    }

    size_t pivot_index = partition_array(arr, low, high);
    quick_sort(arr, low, pivot_index);
    quick_sort(arr, pivot_index + 1, high);
}

static int print_array(const int arr[], size_t size)
{
    for (size_t index = 0; index < size; ++index) {
        if (printf("%d%s", arr[index], index + 1 == size ? "\n" : " ") < 0) {
            return -1;
        }
    }

    if (size == 0 && puts("") == EOF) {
        return -1;
    }

    return 0;
}

int main(void)
{
    const int initial_values[ARRAY_SIZE] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    int *data = malloc(ARRAY_SIZE * sizeof(*data));
    if (data == NULL) {
        (void)fprintf(stderr, "Error: unable to allocate memory for the array.\n");
        return EXIT_FAILURE;
    }

    for (size_t index = 0; index < ARRAY_SIZE; ++index) {
        data[index] = initial_values[index];
    }

    if (puts("Original array:") == EOF || print_array(data, ARRAY_SIZE) != 0) {
        free(data);
        return EXIT_FAILURE;
    }

    quick_sort(data, 0, ARRAY_SIZE);

    if (puts("Sorted array:") == EOF || print_array(data, ARRAY_SIZE) != 0) {
        free(data);
        return EXIT_FAILURE;
    }

    free(data);
    return EXIT_SUCCESS;
}
