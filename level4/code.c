#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *items; /* top to bottom */
    size_t len;
} Pile;

typedef struct {
    size_t rows;
    size_t cols;
    Pile *cells;
} Matrix;

static void free_matrix(Matrix *matrix) {
    if (matrix == NULL) return;

    size_t count = matrix->rows * matrix->cols;
    for (size_t i = 0; i < count; ++i) {
        free(matrix->cells[i].items);
    }

    free(matrix->cells);
    matrix->cells = NULL;
    matrix->rows = matrix->cols = 0;
}

static int is_vowel(char ch) {
    switch (tolower((unsigned char)ch)) {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            return 1;
        default:
            return 0;
    }
}

static int is_consonant(char ch) {
    return isalpha((unsigned char)ch) && !is_vowel(ch);
}

/* Build the result of flipping incoming onto target without changing either. */
static int merge_piles(const Pile *target, const Pile *incoming, Pile *result) {
    char *moving = NULL;

    if (incoming->len > 0) {
        moving = malloc(incoming->len);
        if (moving == NULL) return 0;

        for (size_t i = 0; i < incoming->len; ++i) {
            moving[i] = incoming->items[incoming->len - 1 - i];
        }
    }

    size_t i = incoming->len;
    size_t j = 0;
    int fused = 0;

    while (i > 0 && j < target->len) {
        char a = moving[i - 1];
        char b = target->items[j];

        if ((is_vowel(a) && is_consonant(b)) ||
            (is_consonant(a) && is_vowel(b))) {
            --i;
            ++j;
            continue;
        }

        if (tolower((unsigned char)a) == tolower((unsigned char)b)) {
            fused = 1;
        }
        break;
    }

    size_t length = i + target->len - j - (fused ? 1u : 0u);
    char *items = length > 0 ? malloc(length) : NULL;

    if (length > 0 && items == NULL) {
        free(moving);
        return 0;
    }

    size_t offset = 0;

    if (fused) {
        if (i > 1) {
            memcpy(items, moving, i - 1);
        }
        offset = i - 1;
        items[offset++] = (char)toupper((unsigned char)moving[i - 1]);
        ++j;
    } else {
        if (i > 0) {
            memcpy(items, moving, i);
        }
        offset = i;
    }

    if (target->len > j) {
        memcpy(items + offset, target->items + j, target->len - j);
    }

    free(moving);
    result->items = items;
    result->len = length;
    return 1;
}

static int direction_is(const char *direction, const char *expected) {
    if (direction == NULL) return 0;

    while (*direction && *expected) {
        if (tolower((unsigned char)*direction) !=
            tolower((unsigned char)*expected)) {
            return 0;
        }
        ++direction;
        ++expected;
    }

    return *direction == '\0' && *expected == '\0';
}

/* Apply one fold. A crease cell in an odd-sized dimension stays stationary. */
static int fold_matrix(Matrix *matrix, const char *direction) {
    size_t out_rows = matrix->rows;
    size_t out_cols = matrix->cols;
    size_t row_start = 0;
    size_t col_start = 0;

    int fold_left = direction_is(direction, "left");
    int fold_right = direction_is(direction, "right");
    int fold_up = direction_is(direction, "up");
    int fold_down = direction_is(direction, "down");

    if (fold_left) {
        out_cols = matrix->cols / 2 + matrix->cols % 2;
    } else if (fold_right) {
        col_start = matrix->cols / 2;
        out_cols = matrix->cols - col_start;
    } else if (fold_up) {
        out_rows = matrix->rows / 2 + matrix->rows % 2;
    } else if (fold_down) {
        row_start = matrix->rows / 2;
        out_rows = matrix->rows - row_start;
    } else {
        return 0;
    }

    if (out_rows != 0 && out_cols > SIZE_MAX / out_rows) {
        return 0;
    }

    size_t out_count = out_rows * out_cols;
    Pile *cells = out_count > 0 ? calloc(out_count, sizeof(*cells)) : NULL;
    if (out_count > 0 && cells == NULL) {
        return 0;
    }

    const Pile empty = {NULL, 0};

    for (size_t r = 0; r < out_rows; ++r) {
        for (size_t c = 0; c < out_cols; ++c) {
            size_t target_r = row_start + r;
            size_t target_c = col_start + c;
            size_t source_r = target_r;
            size_t source_c = target_c;

            if (fold_left || fold_right) {
                source_c = matrix->cols - 1 - target_c;
            } else {
                source_r = matrix->rows - 1 - target_r;
            }

            size_t target_index = target_r * matrix->cols + target_c;
            size_t source_index = source_r * matrix->cols + source_c;

            const Pile *incoming = source_index == target_index
                ? &empty
                : &matrix->cells[source_index];

            size_t output_index = r * out_cols + c;

            if (!merge_piles(&matrix->cells[target_index], incoming,
                             &cells[output_index])) {
                Matrix partial = {out_rows, out_cols, cells};
                free_matrix(&partial);
                return 0;
            }
        }
    }

    free_matrix(matrix);
    matrix->rows = out_rows;
    matrix->cols = out_cols;
    matrix->cells = cells;
    return 1;
}

void free_squeezed_grid(char **grid, size_t rows);

/*
 * Fold a rectangular array of input rows (one character per cell).
 * '_' denotes an initially empty cell. The result contains only the top
 * character of each pile, or '_' if the pile is empty.
 *
 * On success, stores allocated NUL-terminated rows in *out_grid and dimensions
 * in *out_rows and *out_cols. Release them with free_squeezed_grid().
 * Returns 1 on success, 0 for invalid input or allocation failure.
 */
int squeeze_matrix(const char *const *grid, size_t rows, size_t cols,
                   const char *const *folds, size_t fold_count,
                   char ***out_grid, size_t *out_rows, size_t *out_cols) {
    if (out_grid == NULL || out_rows == NULL || out_cols == NULL ||
        (rows > 0 && grid == NULL) ||
        (fold_count > 0 && folds == NULL) ||
        (rows > 0 && cols > SIZE_MAX / rows)) {
        return 0;
    }

    *out_grid = NULL;
    *out_rows = 0;
    *out_cols = 0;

    Matrix matrix = {rows, cols, NULL};
    size_t count = rows * cols;

    matrix.cells = count > 0 ? calloc(count, sizeof(*matrix.cells)) : NULL;
    if (count > 0 && matrix.cells == NULL) {
        return 0;
    }

    for (size_t r = 0; r < rows; ++r) {
        if (grid[r] == NULL || strlen(grid[r]) != cols) {
            free_matrix(&matrix);
            return 0;
        }

        for (size_t c = 0; c < cols; ++c) {
            char ch = grid[r][c];
            if (ch == '_') continue;

            Pile *pile = &matrix.cells[r * cols + c];
            pile->items = malloc(1);
            if (pile->items == NULL) {
                free_matrix(&matrix);
                return 0;
            }

            pile->items[0] = ch;
            pile->len = 1;
        }
    }

    for (size_t i = 0; i < fold_count; ++i) {
        if (!fold_matrix(&matrix, folds[i])) {
            free_matrix(&matrix);
            return 0;
        }
    }

    char **view = matrix.rows > 0 ? calloc(matrix.rows, sizeof(*view)) : NULL;
    if (matrix.rows > 0 && view == NULL) {
        free_matrix(&matrix);
        return 0;
    }

    for (size_t r = 0; r < matrix.rows; ++r) {
        if (matrix.cols == SIZE_MAX) {
            free_squeezed_grid(view, r);
            free_matrix(&matrix);
            return 0;
        }

        view[r] = malloc(matrix.cols + 1);
        if (view[r] == NULL) {
            free_squeezed_grid(view, r);
            free_matrix(&matrix);
            return 0;
        }

        for (size_t c = 0; c < matrix.cols; ++c) {
            const Pile *pile = &matrix.cells[r * matrix.cols + c];
            view[r][c] = pile->len > 0 ? pile->items[0] : '_';
        }

        view[r][matrix.cols] = '\0';
    }

    *out_grid = view;
    *out_rows = matrix.rows;
    *out_cols = matrix.cols;

    free_matrix(&matrix);
    return 1;
}

void free_squeezed_grid(char **grid, size_t rows) {
    if (grid == NULL) return;

    for (size_t r = 0; r < rows; ++r) {
        free(grid[r]);
    }
    free(grid);
}
