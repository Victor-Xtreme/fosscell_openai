"""Fold a letter grid and report the visible letter at the top of each pile."""

from collections.abc import Iterable, Sequence

_VOWELS = frozenset("aeiou")
_DIRECTIONS = frozenset({"left", "right", "up", "down"})


def _is_vowel(letter: str) -> bool:
    return letter.casefold() in _VOWELS


def _stack_on(target: list[str], moving: list[str]) -> None:
    """Flip ``moving`` onto ``target`` and resolve reactions at the seam.

    Piles are stored bottom-to-top. A physical flip reverses the moving pile;
    its bottom then meets the target's top. Reactions can expose another pair,
    so resolve the seam until the letters no longer react or a pile is empty.
    """
    incoming = list(reversed(moving))
    next_incoming = 0
    while target and next_incoming < len(incoming):
        lower, upper = target[-1], incoming[next_incoming]
        same_letter = lower.casefold() == upper.casefold()
        opposite_types = _is_vowel(lower) != _is_vowel(upper)

        if same_letter:
            target[-1] = lower.upper()
            next_incoming += 1
        elif opposite_types:
            target.pop()
            next_incoming += 1
        else:
            break

    target.extend(incoming[next_incoming:])


def squeeze_matrix(
    matrix: Sequence[Sequence[str]], folds: Iterable[str]
) -> list[str]:
    """Apply directional folds and return the grid's top-view rows.

    ``matrix`` must be a rectangular grid of single ASCII letters (an empty
    grid returns an empty list).
    Each direction is one of ``left``, ``right``, ``up``, or ``down``. Pile
    contents are stored bottom-to-top, and the moving half is flipped onto the
    stationary half. A vowel and consonant at the seam cancel; matching
    letters (case-insensitively) fuse into one uppercase letter. Reactions
    cascade across the seam. Empty piles appear as underscores in the result.

    Each fold takes O(R*C + P) time, where R*C is the current grid area and P
    is the total number of letters in its piles. The grid uses O(R*C + P)
    space, including pile contents.
    """
    rows = [list(row) for row in matrix]
    if not rows:
        return []
    width = len(rows[0])
    if width == 0 or any(len(row) != width for row in rows):
        raise ValueError("matrix must be non-empty and rectangular")
    if any(not isinstance(ch, str) or len(ch) != 1 or not ch.isascii() or not ch.isalpha()
           for row in rows for ch in row):
        raise ValueError("matrix cells must be single ASCII letters")

    # A cell's list is ordered from the bottom of its pile to the top.
    grid = [[[ch] for ch in row] for row in rows]
    for direction in folds:
        if direction not in _DIRECTIONS:
            raise ValueError(f"unknown fold direction: {direction!r}")

        height, width = len(grid), len(grid[0])
        if direction in ("left", "right"):
            half = (width + 1) // 2
            if direction == "left":
                folded = [[list(grid[r][c]) for c in range(half)] for r in range(height)]
                for r in range(height):
                    for distance in range(width // 2):
                        _stack_on(folded[r][distance], grid[r][width - 1 - distance])
            else:
                start = width // 2
                folded = [[list(grid[r][start + c]) for c in range(half)] for r in range(height)]
                for r in range(height):
                    for distance in range(width // 2):
                        target_col = width - 1 - distance - start
                        _stack_on(folded[r][target_col], grid[r][distance])
            grid = folded
        else:
            half = (height + 1) // 2
            if direction == "up":
                folded = [[list(grid[r][c]) for c in range(width)] for r in range(half)]
                for distance in range(height // 2):
                    source_row = height - 1 - distance
                    for c in range(width):
                        _stack_on(folded[distance][c], grid[source_row][c])
            else:
                start = height // 2
                folded = [[list(grid[start + r][c]) for c in range(width)] for r in range(half)]
                for distance in range(height // 2):
                    target_row = height - 1 - distance - start
                    for c in range(width):
                        _stack_on(folded[target_row][c], grid[distance][c])
            grid = folded

    return ["".join(pile[-1] if pile else "_" for pile in row) for row in grid]
