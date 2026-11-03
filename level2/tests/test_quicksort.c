/*
 * Correctness tests for quick_sort. Every result is checked against the
 * C library's qsort, which proves the output is sorted and is a permutation
 * of the input. Build and run with: make test
 */
#define QUICKSORT_NO_MAIN
#include "../quicksort.c"

#include <limits.h>
#include <stdint.h>

static int tests_run = 0;
static int tests_failed = 0;

/* xorshift64: deterministic across platforms (Windows rand() is only 15 bits). */
static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

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

/* Sorts a copy of `input` both ways and reports any mismatch. */
static void check_case(const char *name, const int input[], size_t size) {
    size_t bytes = (size > 0 ? size : 1) * sizeof(int);
    int *actual = malloc(bytes);
    int *expected = malloc(bytes);
    tests_run++;

    if (actual == NULL || expected == NULL) {
        fprintf(stderr, "FAIL %s: memory allocation failed\n", name);
        tests_failed++;
        free(actual);
        free(expected);
        return;
    }
    if (size > 0) {
        memcpy(actual, input, size * sizeof(int));
        memcpy(expected, input, size * sizeof(int));
    }

    qsort(expected, size, sizeof(int), compare_ints);
    if (quick_sort(actual, size) != 0) {
        fprintf(stderr, "FAIL %s (n=%zu): quick_sort returned an error\n", name, size);
        tests_failed++;
    } else if (size > 0 && memcmp(actual, expected, size * sizeof(int)) != 0) {
        fprintf(stderr, "FAIL %s (n=%zu): output differs from qsort\n", name, size);
        tests_failed++;
    }

    free(actual);
    free(expected);
}

static void test_bad_input(void) {
    tests_run++;
    if (quick_sort(NULL, 5) != -1) {
        fprintf(stderr, "FAIL null array: expected -1 for NULL with size 5\n");
        tests_failed++;
    }
    tests_run++;
    if (quick_sort(NULL, 0) != 0) {
        fprintf(stderr, "FAIL null array: expected 0 for NULL with size 0\n");
        tests_failed++;
    }
}

static void test_fixed_cases(void) {
    const int one[] = {42};
    const int two_sorted[] = {1, 2};
    const int two_reversed[] = {2, 1};
    const int extremes[] = {INT_MAX, 0, INT_MIN, -1, 1, INT_MAX, INT_MIN};
    const int demo[] = {10, 7, 8, 9, 1, 5, 2, 3, 4, 6};

    check_case("empty", one, 0);
    check_case("one element", one, 1);
    check_case("two sorted", two_sorted, 2);
    check_case("two reversed", two_reversed, 2);
    check_case("INT_MIN/INT_MAX", extremes, sizeof(extremes) / sizeof(extremes[0]));
    check_case("demo array", demo, sizeof(demo) / sizeof(demo[0]));
}

/* Structured inputs that break naive quicksorts, at several sizes. */
static void test_patterns(void) {
    const size_t sizes[] = {15, 16, 17, 100, 1000, 100000};
    const size_t max_size = 100000;
    int *buf = malloc(max_size * sizeof(int));
    if (buf == NULL) {
        fprintf(stderr, "FAIL patterns: memory allocation failed\n");
        tests_failed++;
        return;
    }

    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        size_t n = sizes[s];

        for (size_t i = 0; i < n; i++) buf[i] = (int)i;
        check_case("ascending", buf, n);

        for (size_t i = 0; i < n; i++) buf[i] = (int)(n - i);
        check_case("descending", buf, n);

        for (size_t i = 0; i < n; i++) buf[i] = 7;
        check_case("all equal", buf, n);

        for (size_t i = 0; i < n; i++) buf[i] = (int)(i < n / 2 ? i : n - i);
        check_case("organ pipe", buf, n);

        for (size_t i = 0; i < n; i++) buf[i] = (int)(i % 10);
        check_case("sawtooth", buf, n);

        for (size_t i = 0; i < n; i++) buf[i] = (int)(next_random() % 4) - 2;
        check_case("few unique, negative", buf, n);
    }
    free(buf);
}

/* Random arrays of random sizes, mixing narrow and full int ranges. */
static void test_random(void) {
    const size_t rounds = 2000;
    const size_t max_size = 3000;
    int *buf = malloc(max_size * sizeof(int));
    if (buf == NULL) {
        fprintf(stderr, "FAIL random: memory allocation failed\n");
        tests_failed++;
        return;
    }

    for (size_t r = 0; r < rounds; r++) {
        size_t n = (size_t)(next_random() % max_size);
        int narrow = (r % 2 == 0);
        for (size_t i = 0; i < n; i++) {
            uint64_t v = next_random();
            buf[i] = narrow ? (int)(v % 50) : (int)(uint32_t)v;
        }
        check_case(narrow ? "random narrow" : "random full range", buf, n);
    }
    free(buf);
}

int main(void) {
    test_bad_input();
    test_fixed_cases();
    test_patterns();
    test_random();

    printf("%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
