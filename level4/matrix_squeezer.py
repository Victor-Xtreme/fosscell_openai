"""Fold a rectangular letter grid while preserving bottom-to-top piles."""

from string import ascii_letters


def _merge(target, moving):
    """Flip the moving pile onto target, resolving each contact in order."""
    for letter in reversed(moving):
        target.append(letter)
        while len(target) >= 2:
            below, above = target[-2:]
            if below.lower() == above.lower():
                target[-2:] = [above.upper()]
            elif (below.lower() in "aeiou") != (above.lower() in "aeiou"):
                del target[-2:]
            else:
                break


def fold_matrix(matrix, directions):
    """Return the top view as a list of row strings after folding.

    ``matrix`` is a nonempty rectangular iterable of rows, each containing
    single ASCII letters. ``directions`` is an iterable of ``left``, ``right``,
    ``up``, or ``down`` strings (case-insensitive). Inputs are not modified.
    Piles flip in reverse order onto the stationary half. Odd-sized folds keep
    the middle row/column unchanged; folding an axis of length one is a no-op.
    Matching letters fuse to uppercase, irrespective of case. Vowel/consonant
    contacts annihilate; other contacts stack. Y is a consonant. Contacts resolve
    repeatedly until stable. Empty piles display as underscores.

    Raises TypeError for non-iterable inputs or a bare directions string, and
    ValueError for empty/ragged grids, invalid cells, or unknown directions.
    """
    rows = [list(row) for row in matrix]
    if not rows or not rows[0] or any(len(row) != len(rows[0]) for row in rows):
        raise ValueError("matrix must be nonempty and rectangular")
    if any(not isinstance(cell, str) or len(cell) != 1 or cell not in ascii_letters
           for row in rows for cell in row):
        raise ValueError("matrix cells must be single ASCII letters")
    if isinstance(directions, str):
        raise TypeError("directions must be an iterable of direction strings")
    folds = list(directions)
    if any(not isinstance(direction, str) or direction.lower() not in
           {"left", "right", "up", "down"} for direction in folds):
        raise ValueError("directions must be left, right, up, or down")

    board = [[[letter] for letter in row] for row in rows]
    for direction in folds:
        direction = direction.lower()
        vertical = direction in {"up", "down"}
        if vertical:
            board = [list(column) for column in zip(*board)]

        width = len(board[0])
        half = width // 2
        kept = width - half
        folded = []
        for row in board:
            if direction in {"left", "up"}:
                stationary = row[:kept]
                for column in range(half):
                    _merge(stationary[column], row[width - 1 - column])
            else:
                stationary = row[half:]
                for column in range(half):
                    _merge(stationary[kept - 1 - column], row[column])
            folded.append(stationary)
        board = [list(column) for column in zip(*folded)] if vertical else folded

    return ["".join(pile[-1] if pile else "_" for pile in row) for row in board]
