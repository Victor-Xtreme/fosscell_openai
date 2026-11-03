/*
 * Times quick_sort against the C library's qsort on 1,000,000 ints in
 * several input patterns. Build and run with: make bench
 */
#define QUICKSORT_NO_MAIN
#include "../quicksort.c"

#include <stdint.h>
#include <time.h>

#define BENCH_SIZE 1000000

static uint64_t rng_state = 0x2545F4914F6CDD1DULL;

static uint64_t next_random(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static int compare_ints(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static void fill_random(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = (int)(uint32_t)next_random();
}
static void fill_ascending(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = (int)i;
}
static void fill_descending(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = (int)(n - i);
}
static void fill_few_unique(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = (int)(next_random() % 8);
}
static void fill_all_equal(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = 7;
}
static void fill_organ_pipe(int arr[], size_t n) {
    for (size_t i = 0; i < n; i++) arr[i] = (int)(i < n / 2 ? i : n - i);
}

static double elapsed_ms(clock_t start) {
    return 1000.0 * (double)(clock() - start) / CLOCKS_PER_SEC;
}

int main(void) {
    struct {
        const char *name;
        void (*fill)(int[], size_t);
    } patterns[] = {
        {"random", fill_random},
        {"ascending", fill_ascending},
        {"descending", fill_descending},
        {"few unique (8)", fill_few_unique},
        {"all equal", fill_all_equal},
        {"organ pipe", fill_organ_pipe},
    };

    int *original = malloc(BENCH_SIZE * sizeof(int));
    int *work = malloc(BENCH_SIZE * sizeof(int));
    if (original == NULL || work == NULL) {
        fprintf(stderr, "Error: memory allocation failed\n");
        free(original);
        free(work);
        return EXIT_FAILURE;
    }

    printf("Sorting %d ints (times in ms)\n\n", BENCH_SIZE);
    printf("%-16s %12s %12s %9s\n", "pattern", "quick_sort", "qsort", "speedup");

    for (size_t p = 0; p < sizeof(patterns) / sizeof(patterns[0]); p++) {
        patterns[p].fill(original, BENCH_SIZE);

        memcpy(work, original, BENCH_SIZE * sizeof(int));
        clock_t start = clock();
        if (quick_sort(work, BENCH_SIZE) != 0) {
            fprintf(stderr, "Error: quick_sort failed\n");
            free(original);
            free(work);
            return EXIT_FAILURE;
        }
        double ours = elapsed_ms(start);

        memcpy(work, original, BENCH_SIZE * sizeof(int));
        start = clock();
        qsort(work, BENCH_SIZE, sizeof(int), compare_ints);
        double theirs = elapsed_ms(start);

        /* clock() has ~1 ms resolution; treat a 0 ms run as 1 ms (a lower bound). */
        if (ours > 0.0) {
            printf("%-16s %12.1f %12.1f %8.1fx\n", patterns[p].name, ours, theirs,
                   theirs / ours);
        } else {
            printf("%-16s %12s %12.1f %7s%.0fx\n", patterns[p].name, "<1", theirs,
                   ">", theirs);
        }
    }

    free(original);
    free(work);
    return EXIT_SUCCESS;
}
