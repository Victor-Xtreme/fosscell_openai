"""Fold a grid of letter piles and return the visible top letter per cell."""

from typing import Iterable, Sequence, Union

_VOWELS = frozenset("aeiou")
_DIRECTIONS = frozenset(("left", "right", "up", "down"))


def _pile(value: Union[str, Iterable[str]]) -> list:
    """Normalize a cell into a bottom-to-top pile of single ASCII letters."""
    if isinstance(value, str):
        letters = list(value)
    else:
        try:
            letters = list(value)
        except TypeError as exc:
            raise TypeError("each cell must be a string or iterable of letters") from exc

    if any(not isinstance(letter, str) or len(letter) != 1 or not letter.isascii()
           or not letter.isalpha() for letter in letters):
        raise ValueError("cell piles must contain single ASCII letters only")
    return letters


def _combine(target: list, moving: list) -> list:
    """Flip moving over target and resolve reactions along the contact seam."""
    incoming = list(reversed(moving))

    while target and incoming:
        lower = target[-1]
        upper = incoming[0]
        lower_is_vowel = lower.lower() in _VOWELS
        upper_is_vowel = upper.lower() in _VOWELS

        if lower.lower() == upper.lower():
            target.pop()
            incoming.pop(0)
            target.append(lower.upper())
            break

        if lower_is_vowel != upper_is_vowel:
            target.pop()
            incoming.pop(0)
            continue

        break

    return target + incoming


def _fold_left(board: list) -> list:
    width = len(board[0])
    half = width // 2
    result = []
    for row in board:
        folded = [_combine(row[index], row[width - 1 - index])
                  for index in range(half)]
        if width % 2:
            folded.append(row[half])
        result.append(folded)
    return result


def _fold_right(board: list) -> list:
    width = len(board[0])
    half = width // 2
    start = width - half
    result = []
    for row in board:
        folded = [_combine(row[index], row[width - 1 - index])
                  for index in range(start, width)]
        if width % 2:
            folded.insert(0, row[half])
        result.append(folded)
    return result


def _fold_up(board: list) -> list:
    height = len(board)
    half = height // 2
    result = []
    for row_index in range(half):
        result.append([_combine(board[row_index][column],
                                board[height - 1 - row_index][column])
                       for column in range(len(board[0]))])
    if height % 2:
        result.append(board[half])
    return result


def _fold_down(board: list) -> list:
    height = len(board)
    half = height // 2
    start = height - half
    result = []
    for row_index in range(start, height):
        result.append([_combine(board[row_index][column],
                                board[height - 1 - row_index][column])
                       for column in range(len(board[0]))])
    if height % 2:
        result.insert(0, board[half])
    return result


def squeeze_matrix(grid: Sequence[Sequence[Union[str, Iterable[str]]]],
                   folds: Iterable[str]) -> list:
    """Fold a rectangular letter grid and return its top view.

    Each cell is either a string or an iterable of letters arranged
    bottom-to-top. For each fold, the named side is folded onto the opposite
    side. A cell pile that moves is reversed before it lands. Reactions resolve
    inward from the contact seam: unlike vowel/consonant pairs disappear, and
    identical letters fuse into one uppercase letter. Odd-sized dimensions keep
    the middle row or column as a stationary crease.

    The returned strings show the top letter of each remaining pile. Empty
    piles appear as underscores. The top of a pile is its final character.

    Raises:
        TypeError: if the grid or folds are not iterable as expected.
        ValueError: if the grid is empty, ragged, contains invalid letters, or
            a fold direction is not left, right, up, or down.
    """
    try:
        rows = [list(row) for row in grid]
    except TypeError as exc:
        raise TypeError("grid must be an iterable of rows") from exc

    if not rows:
        raise ValueError("grid must contain at least one row")

    width = len(rows[0])
    if width == 0 or any(len(row) != width for row in rows):
        raise ValueError("grid must be a non-empty rectangle")

    board = [[_pile(cell) for cell in row] for row in rows]
    try:
        fold_list = [direction.lower() for direction in folds]
    except (AttributeError, TypeError) as exc:
        raise TypeError("folds must be an iterable of direction strings") from exc

    fold_functions = {
        "left": _fold_left,
        "right": _fold_right,
        "up": _fold_up,
        "down": _fold_down,
    }
    for direction in fold_list:
        if direction not in _DIRECTIONS:
            raise ValueError(f"unknown fold direction: {direction!r}")
        board = fold_functions[direction](board)

    return ["".join(cell[-1] if cell else "_" for cell in row)
            for row in board]
