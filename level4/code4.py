
VOWELS = set("aeiouAEIOU")


def interact(stack, incoming):
    """Merge two stacks, applying letter interaction rules."""

    result = stack[:]

    for char in incoming:
        if not result:
            result.append(char)
            continue

        top = result[-1]

        # Vowel and consonant cancel each other
        if ((top in VOWELS and char.isalpha() and char not in VOWELS)
                or (char in VOWELS and top.isalpha() and top not in VOWELS)):
            result.pop()

        # Identical letters fuse into uppercase
        elif top.lower() == char.lower():
            result[-1] = char.upper()

        else:
            result.append(char)

    return result


def fold_left(grid):
    rows = len(grid)
    cols = len(grid[0])

    new_cols = (cols + 1) // 2
    result = [[[] for _ in range(new_cols)] for _ in range(rows)]

    # Copy the left side, including the crease if present
    for r in range(rows):
        for c in range(new_cols):
            result[r][c] = grid[r][c][:]

    # Fold the right side onto the left
    for r in range(rows):
        for c in range(cols // 2):
            source = cols - 1 - c
            incoming = grid[r][source][::-1]

            result[r][c] = interact(result[r][c], incoming)

    return result


def fold_right(grid):
    rows = len(grid)
    cols = len(grid[0])

    new_cols = (cols + 1) // 2
    start = cols // 2

    result = [[[] for _ in range(new_cols)] for _ in range(rows)]

    # Copy the right side, including the crease if present
    for r in range(rows):
        for c in range(new_cols):
            result[r][c] = grid[r][start + c][:]

    # Fold the left side onto the right
    for r in range(rows):
        for c in range(cols // 2):
            source = cols // 2 - 1 - c
            target = cols - 1 - c

            incoming = grid[r][source][::-1]
            result[r][target - start] = interact(
                result[r][target - start], incoming
            )

    return result


def fold_up(grid):
    rows = len(grid)
    cols = len(grid[0])

    new_rows = (rows + 1) // 2
    result = [[[] for _ in range(cols)] for _ in range(new_rows)]

    # Copy the upper side, including the crease if present
    for r in range(new_rows):
        for c in range(cols):
            result[r][c] = grid[r][c][:]

    # Fold the bottom side onto the top
    for r in range(rows // 2):
        source = rows - 1 - r

        for c in range(cols):
            incoming = grid[source][c][::-1]
            result[r][c] = interact(result[r][c], incoming)

    return result


def fold_down(grid):
    rows = len(grid)
    cols = len(grid[0])

    new_rows = (rows + 1) // 2
    start = rows // 2

    result = [[[] for _ in range(cols)] for _ in range(new_rows)]

    # Copy the lower side, including the crease if present
    for r in range(new_rows):
        for c in range(cols):
            result[r][c] = grid[start + r][c][:]

    # Fold the upper side onto the bottom
    for r in range(rows // 2):
        source = rows // 2 - 1 - r
        target = rows - 1 - r

        for c in range(cols):
            incoming = grid[source][c][::-1]
            result[target - start][c] = interact(
                result[target - start][c], incoming
            )

    return result


def matrix_squeezer(matrix, folds):
    """Apply a sequence of folds to a character matrix."""

    if not matrix or not matrix[0]:
        raise ValueError("Matrix cannot be empty")

    cols = len(matrix[0])

    if any(len(row) != cols for row in matrix):
        raise ValueError("Matrix must be rectangular")

    # Convert every character into a stack
    grid = [
        [[char] for char in row]
        for row in matrix
    ]

    operations = {
        "left": fold_left,
        "right": fold_right,
        "up": fold_up,
        "down": fold_down
    }

    for direction in folds:
        direction = direction.lower()

        if direction not in operations:
            raise ValueError(f"Invalid fold direction: {direction}")

        grid = operations[direction](grid)

    # Display only the top character of each remaining stack
    return [
        "".join(stack[-1] if stack else "_" for stack in row)
        for row in grid
    ]


# Example
matrix = [
    ["a", "b", "c", "d"],
    ["e", "f", "g", "h"],
    ["i", "j", "k", "l"],
    ["m", "n", "o", "p"]
]

folds = ["left", "up"]

result = matrix_squeezer(matrix, folds)

print("Final matrix:")
for row in result:
    print(row)
