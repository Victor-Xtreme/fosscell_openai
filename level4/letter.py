"""Fold a letter matrix while preserving and reacting its hidden layers."""

VOWELS = frozenset("aeiou")
DIRECTIONS = frozenset(("left", "right", "up", "down"))


def _overlay(target, source):
    """Flip source onto target; both lists are stored bottom-to-top."""
    while target and source:
        lower, upper = target[-1].lower(), source[-1].lower()
        if lower == upper:
            target[-1] = source.pop().upper()
        elif (lower in VOWELS) != (upper in VOWELS):
            target.pop()
            source.pop()
        else:
            break
    target.extend(reversed(source))


def squeeze_matrix(matrix, folds):
    """Return the folded top view as a list of strings, using '_' for empty cells.

    Input is a rectangular iterable of rows of single ASCII letters. Directions
    are left/right/up/down (case-insensitive). Odd dimensions retain the crease
    row/column. Letter identity and vowel classification ignore case; fusion
    produces uppercase. Reactions cascade at the contact boundary. Inputs are
    not mutated. An empty matrix returns []; empty rows are rejected.
    """
    rows = [list(row) for row in matrix]
    directions = list(folds)
    if any(not isinstance(d, str) or d.lower() not in DIRECTIONS
           for d in directions):
        raise ValueError("folds must contain left, right, up, or down")
    if not rows:
        return []
    width = len(rows[0])
    if not width or any(len(row) != width for row in rows):
        raise ValueError("matrix must be rectangular with nonempty rows")
    if any(not isinstance(c, str) or len(c) != 1 or not c.isascii()
           or not c.isalpha() for row in rows for c in row):
        raise ValueError("each cell must be one ASCII letter")

    board = [[[letter] for letter in row] for row in rows]
    for direction in directions:
        direction = direction.lower()
        vertical = direction in ("up", "down")
        # Transposition lets the same folding logic handle either axis.
        lines = [list(column) for column in zip(*board)] if vertical else board
        folded = []
        for line in lines:
            size = len(line)
            half = size // 2
            keep = (size + 1) // 2
            if direction in ("left", "up"):
                result = line[:keep]
                for index in range(half):
                    _overlay(result[index], line[size - 1 - index])
            else:
                result = line[half:]
                for index in range(half):
                    _overlay(result[keep - 1 - index], line[index])
            folded.append(result)
        board = [list(row) for row in zip(*folded)] if vertical else folded
    return ["".join(stack[-1] if stack else "_" for stack in row)
            for row in board]
