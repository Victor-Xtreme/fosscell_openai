
def is_vowel(letter):
    return letter.lower() in "aeiou"


def stack_piles(target, source):
    """
    Piles are stored bottom-to-top.
    Flip the source pile, place it on the target,
    and resolve collisions at the contact point.
    """
    base = target.copy()
    incoming = list(reversed(source))

    i = 0

    while base and i < len(incoming):
        a = base[-1]
        b = incoming[i]

        # Vowel + consonant: both disappear
        if is_vowel(a) != is_vowel(b):
            base.pop()
            i += 1

        # Identical letters: fuse into uppercase
        elif a.lower() == b.lower():
            base[-1] = a.upper()
            return base + incoming[i + 1:]

        else:
            break

    return base + incoming[i:]


def fold_left(grid):
    rows = len(grid)
    cols = len(grid[0])

    keep = (cols + 1) // 2

    result = [
        [pile.copy() for pile in row[:keep]]
        for row in grid
    ]

    for r in range(rows):
        for c in range(cols // 2):
            source = grid[r][cols - 1 - c]
            result[r][c] = stack_piles(result[r][c], source)

    return result


def fold_right(grid):
    rows = len(grid)
    cols = len(grid[0])

    start = cols // 2

    result = [
        [pile.copy() for pile in row[start:]]
        for row in grid
    ]

    for r in range(rows):
        for c in range(cols // 2):
            source = grid[r][c]
            target = cols - 1 - c

            result[r][target - start] = stack_piles(
                result[r][target - start],
                source
            )

    return result


def squeeze_matrix(matrix, directions):
    """
    Apply a sequence of folds and return the final top view.

    Each cell can be:
      - A single letter, e.g. "a"
      - A pile represented bottom-to-top, e.g. "abc"
      - A list of letters, e.g. ["a", "b", "c"]

    Empty piles are represented by "" or [].
    """

    if not matrix:
        raise ValueError("Matrix cannot be empty")

    if isinstance(directions, str):
        raise TypeError("Directions must be a sequence of directions")

    # Normalize input into a matrix of piles
    grid = []

    for row in matrix:
        cells = list(row)

        normalized_row = []

        for cell in cells:
            try:
                pile = list(cell)
            except TypeError:
                raise TypeError("Each cell must contain letters")

            if any(
                not isinstance(ch, str)
                or len(ch) != 1
                or not ch.isascii()
                or not ch.isalpha()
                for ch in pile
            ):
                raise ValueError("Piles must contain single English letters")

            normalized_row.append(pile)

        grid.append(normalized_row)

    cols = len(grid[0])

    if cols == 0 or any(len(row) != cols for row in grid):
        raise ValueError("Matrix must be non-empty and rectangular")

    # Apply folds in the specified order
    for direction in directions:
        direction = direction.lower()

        if direction == "left":
            grid = fold_left(grid)

        elif direction == "right":
            grid = fold_right(grid)

        elif direction == "up":
            # Transpose, fold left, then transpose back
            grid = [list(row) for row in zip(*grid)]
            grid = fold_left(grid)
            grid = [list(row) for row in zip(*grid)]

        elif direction == "down":
            # Transpose, fold right, then transpose back
            grid = [list(row) for row in zip(*grid)]
            grid = fold_right(grid)
            grid = [list(row) for row in zip(*grid)]

        else:
            raise ValueError(f"Invalid fold direction: {direction}")

    # Return only the topmost letter of each pile
    return [
        [pile[-1] if pile else "_" for pile in row]
        for row in grid
    ]
