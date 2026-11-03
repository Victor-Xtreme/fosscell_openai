
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

typedef struct {
    int rows;
    int cols;
    char ***cell;
} Matrix;

/* ---------- BASIC HELPERS ---------- */

int isVowel(char c) {
    c = (char)tolower((unsigned char)c);

    return c == 'a' || c == 'e' || c == 'i' ||
           c == 'o' || c == 'u';
}

int isLetter(char c) {
    return isalpha((unsigned char)c);
}

int isVowelConsonant(char a, char b) {
    return isLetter(a) && isLetter(b) &&
           isVowel(a) != isVowel(b);
}

char *copyPile(const char *pile) {
    if (pile == NULL)
        pile = "";

    size_t len = strlen(pile);

    char *copy = malloc(len + 1);

    if (copy != NULL)
        memcpy(copy, pile, len + 1);

    return copy;
}

/* ---------- MEMORY MANAGEMENT ---------- */

int allocateMatrix(Matrix *m, int rows, int cols) {
    if (m == NULL || rows <= 0 || cols <= 0)
        return 0;

    m->rows = 0;
    m->cols = 0;
    m->cell = NULL;

    if ((size_t)rows > SIZE_MAX / sizeof(char **))
        return 0;

    if ((size_t)cols > SIZE_MAX / sizeof(char *))
        return 0;

    m->cell = calloc((size_t)rows, sizeof(char **));

    if (m->cell == NULL)
        return 0;

    for (int i = 0; i < rows; i++) {
        m->cell[i] = calloc((size_t)cols, sizeof(char *));

        if (m->cell[i] == NULL) {
            for (int k = 0; k < i; k++)
                free(m->cell[k]);

            free(m->cell);
            m->cell = NULL;
            return 0;
        }
    }

    m->rows = rows;
    m->cols = cols;

    return 1;
}

void freeMatrix(Matrix *m) {
    if (m == NULL || m->cell == NULL)
        return;

    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++)
            free(m->cell[i][j]);

        free(m->cell[i]);
    }

    free(m->cell);

    m->cell = NULL;
    m->rows = 0;
    m->cols = 0;
}

/* ---------- PILE INTERACTIONS ---------- */

/*
   Piles are stored bottom-to-top.

   Example:
   "ABC" means A is at the bottom and C is at the top.

   The incoming pile is reversed before being placed
   on top of the target pile.
*/
char *combinePiles(const char *target, const char *source) {
    if (target == NULL)
        target = "";

    if (source == NULL)
        source = "";

    size_t tLen = strlen(target);
    size_t sLen = strlen(source);

    if (tLen > SIZE_MAX - sLen - 1)
        return NULL;

    size_t capacity = tLen + sLen + 1;

    char *stack = malloc(capacity);

    if (stack == NULL)
        return NULL;

    /* Copy the target pile first. */
    memcpy(stack, target, tLen);

    size_t top = tLen;

    /*
       Process the source pile in reverse order.
       This simulates flipping it upside down.
    */
    for (size_t i = sLen; i > 0; i--) {
        char incoming = source[i - 1];

        /* No letter underneath: simply place it. */
        if (top == 0) {
            stack[top++] = incoming;
            continue;
        }

        char existing = stack[top - 1];

        /* Vowel + consonant: both disappear. */
        if (isVowelConsonant(existing, incoming)) {
            top--;
            continue;
        }

        /* Identical letters fuse into uppercase. */
        if (isLetter(existing) &&
            isLetter(incoming) &&
            toupper((unsigned char)existing) ==
            toupper((unsigned char)incoming)) {

            stack[top - 1] =
                (char)toupper((unsigned char)existing);

            continue;
        }

        /* No reaction: incoming letter sits on top. */
        stack[top++] = incoming;
    }

    stack[top] = '\0';

    return stack;
}

/* ---------- MATRIX INITIALIZATION ---------- */

int initializeMatrix(Matrix *m, int rows, int cols,
                     const char *initial[]) {

    if (!allocateMatrix(m, rows, cols))
        return 0;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            m->cell[i][j] =
                copyPile(initial[i * cols + j]);

            if (m->cell[i][j] == NULL) {
                freeMatrix(m);
                return 0;
            }
        }
    }

    return 1;
}

/* ---------- FOLDING OPERATION ---------- */

int foldOnce(Matrix *m, const char *direction) {
    if (m == NULL || m->cell == NULL || direction == NULL)
        return 0;

    int left  = strcmp(direction, "left") == 0;
    int right = strcmp(direction, "right") == 0;
    int up    = strcmp(direction, "up") == 0;
    int down  = strcmp(direction, "down") == 0;

    if (!left && !right && !up && !down)
        return 0;

    int horizontal = left || right;

    int newRows = down || up
        ? m->rows / 2 + m->rows % 2
        : m->rows;

    int newCols = horizontal
        ? m->cols / 2 + m->cols % 2
        : m->cols;

    Matrix result = {0};

    if (!allocateMatrix(&result, newRows, newCols))
        return 0;

    for (int i = 0; i < newRows; i++) {
        for (int j = 0; j < newCols; j++) {

            int targetRow = i;
            int targetCol = j;

            if (right)
                targetCol = m->cols - newCols + j;

            if (down)
                targetRow = m->rows - newRows + i;

            int crease = 0;

            if (horizontal &&
                m->cols % 2 == 1 &&
                targetCol == m->cols / 2)
                crease = 1;

            if ((up || down) &&
                m->rows % 2 == 1 &&
                targetRow == m->rows / 2)
                crease = 1;

            const char *target =
                m->cell[targetRow][targetCol];

            if (crease) {
                /* Crease stays exactly as it is. */
                result.cell[i][j] = copyPile(target);
            } else {
                int sourceRow = targetRow;
                int sourceCol = targetCol;

                if (horizontal)
                    sourceCol = m->cols - 1 - targetCol;

                if (up || down)
                    sourceRow = m->rows - 1 - targetRow;

                const char *source =
                    m->cell[sourceRow][sourceCol];

                result.cell[i][j] =
                    combinePiles(target, source);
            }

            if (result.cell[i][j] == NULL) {
                freeMatrix(&result);
                return 0;
            }
        }
    }

    /* Replace the old matrix only after successful folding. */
    freeMatrix(m);
    *m = result;

    return 1;
}

/* ---------- MULTIPLE FOLDS ---------- */

int squeezeMatrix(Matrix *m,
                  const char *directions[],
                  int foldCount) {

    if (m == NULL || m->cell == NULL ||
        foldCount < 0 ||
        (foldCount > 0 && directions == NULL))
        return 0;

    for (int i = 0; i < foldCount; i++) {
        if (!foldOnce(m, directions[i])) {
            fprintf(stderr,
                    "Error: Invalid fold or memory allocation failure.\n");
            return 0;
        }
    }

    return 1;
}

/* ---------- FINAL TOP VIEW ---------- */

void printMatrix(const Matrix *m) {
    if (m == NULL || m->cell == NULL)
        return;

    printf("\nFinal Matrix:\n");

    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {

            const char *pile = m->cell[i][j];

            if (pile == NULL || pile[0] == '\0')
                printf("_");
            else
                printf("%s", pile);

            printf("\t");
        }

        printf("\n");
    }
}

/* ---------- MAIN ---------- */

int main(void) {

    const int ROWS = 3;
    const int COLS = 3;

    const char *initial[] = {
        "A", "B", "C",
        "D", "E", "F",
        "G", "H", "I"
    };

    const char *folds[] = {
        "left",
        "up"
    };

    const int foldCount =
        sizeof(folds) / sizeof(folds[0]);

    Matrix matrix = {0};

    if (!initializeMatrix(&matrix, ROWS, COLS, initial)) {
        fprintf(stderr, "Matrix initialization failed.\n");
        return EXIT_FAILURE;
    }

    if (!squeezeMatrix(&matrix, folds, foldCount)) {
        freeMatrix(&matrix);
        return EXIT_FAILURE;
    }

    printMatrix(&matrix);

    freeMatrix(&matrix);

    return EXIT_SUCCESS;
}
