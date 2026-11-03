#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 1000

/* Return codes */
#define SORT_OK        0
#define SORT_ERR_ARG  -1

static void swap_elements(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Lomuto partition using median-of-three pivot to avoid O(n^2)
 * behavior on already-sorted input. */
static int partition_array(int arr[], int low, int high)
{
    int mid = low + (high - low) / 2;

    /* Order arr[low], arr[mid], arr[high]; median ends up at mid. */
    if (arr[mid] < arr[low])  swap_elements(&arr[mid], &arr[low]);
    if (arr[high] < arr[low]) swap_elements(&arr[high], &arr[low]);
    if (arr[high] < arr[mid]) swap_elements(&arr[high], &arr[mid]);

    /* Move the median to high to use as the pivot. */
    swap_elements(&arr[mid], &arr[high]);

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

static void quick_sort_range(int arr[], int low, int high)
{
    /* Recurse into the smaller partition, loop on the larger one,
     * which bounds stack depth to O(log n). */
    while (low < high) {
        int pi = partition_array(arr, low, high);

        if (pi - low < high - pi) {
            quick_sort_range(arr, low, pi - 1);
            low = pi + 1;
        } else {
            quick_sort_range(arr, pi + 1, high);
            high = pi - 1;
        }
    }
}

/* Public entry point. Returns SORT_OK or SORT_ERR_ARG. */
int quick_sort(int arr[], int size)
{
    if (arr == NULL || size < 0 || size > MAX_SIZE) {
        return SORT_ERR_ARG;
    }
    if (size > 1) {
        quick_sort_range(arr, 0, size - 1);
    }
    return SORT_OK;
}

static void print_array(const int arr[], int size)
{
    for (int i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void)
{
    static const int initial[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};
    const int count = (int)(sizeof(initial) / sizeof(initial[0]));

    int *data = malloc((size_t)count * sizeof(*data));
    if (data == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < count; i++) {
        data[i] = initial[i];
    }

    printf("Original array:\n");
    print_array(data, count);

    if (quick_sort(data, count) != SORT_OK) {
        fprintf(stderr, "Error: invalid arguments to quick_sort\n");
        free(data);
        return EXIT_FAILURE;
    }

    printf("Sorted array:\n");
    print_array(data, count);

    free(data);
    return EXIT_SUCCESS;
}