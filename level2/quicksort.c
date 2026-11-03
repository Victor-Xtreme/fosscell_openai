#include <stdio.h>
#include <stdlib.h>
/* Removed unused <time.h> and MAX_SIZE. */

/* Changed all function names to snake_case.
   Added static because these functions are used only in this file.
   Used consistent indentation and brace placement. */
static void swap_elements(int *a, int *b)
{
    int temp = *a; /* Renamed t to temp for clarity. */
    *a = *b;
    *b = temp;
}

static int partition_array(int array[], int low, int high)
{
    int pivot = array[high];
    int i = low; /* Simplified indexing: i marks the next smaller element. */

    for (int j = low; j < high; ++j) {
        if (array[j] < pivot) {
            swap_elements(&array[i], &array[j]);
            ++i;
        }
    }

    swap_elements(&array[i], &array[high]);
    return i;
}

static void quick_sort(int array[], int low, int high)
{
    /* Replaced two recursive calls with one recursive call and a loop.
       Recursing on the smaller partition limits stack usage. */
    while (low < high) {
        int pivot_index = partition_array(array, low, high);

        if (pivot_index - low < high - pivot_index) {
            quick_sort(array, low, pivot_index - 1);
            low = pivot_index + 1;
        } else {
            quick_sort(array, pivot_index + 1, high);
            high = pivot_index - 1;
        }
    }
}

/* Added const because printing should not modify the array. */
static void print_array(const int array[], int size)
{
    for (int i = 0; i < size; ++i) {
        /* Print spaces between numbers without a trailing space. */
        printf("%s%d", i == 0 ? "" : " ", array[i]);
    }
    putchar('\n');
}

/* Used main(void) to explicitly indicate no parameters. */
int main(void)
{
    /* Replaced separate assignments with an initializer. */
    const int initial_data[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};

    /* Calculate the size instead of repeating the hardcoded value 10. */
    const int size = (int)(sizeof initial_data / sizeof initial_data[0]);

    /* Allocate enough memory for all initial elements.
       Removed the unnecessary malloc cast in C. */
    int *data = malloc(sizeof initial_data);

    /* Check allocation before accessing data to avoid undefined behavior. */
    if (data == NULL) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    /* Copy the initial values into the allocated array. */
    for (int i = 0; i < size; ++i) {
        data[i] = initial_data[i];
    }

    /* Used puts for plain text followed by a newline. */
    puts("Original array:");
    print_array(data, size);

    quick_sort(data, 0, size - 1);

    puts("Sorted array:");
    print_array(data, size);

    free(data); /* Added free to prevent the memory leak. */
    return EXIT_SUCCESS; /* Used named success/failure constants consistently. */
}
