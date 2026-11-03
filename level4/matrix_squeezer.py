"""Fold a letter matrix and return the visible top letter in each cell.

Piles are stored bottom-to-top. A source pile is reversed as it flips, then
placed on the target pile. Reactions happen at the contact surface: a vowel
and a consonant cancel; matching letters fuse to uppercase. If cancellation
opens the contact surface, the next pair can react as well. A plain stack or
fusion stops further reactions, so letters already stacked together inside
either pile do not react with one another.

Example:
    >>> squeeze_matrix([["a", "a"]], ["left"])
    [["A"]]
"""

from typing import Iterable, List, Sequence

Pile = List[str]
Board = List[List[Pile]]
VOWELS = frozenset("aeiou")
DIRECTIONS = frozenset(("left", "right", "up", "down"))


def _make_board(matrix: Sequence[Sequence[str]]) -> Board:
    rows = [list(row) for row in matrix]
    if not rows:
        raise ValueError("matrix must contain at least one row")

    width = len(rows[0])
    if width == 0:
        raise ValueError("matrix rows must not be empty")
    if any(len(row) != width for row in rows):
        raise ValueError("matrix must be rectangular")

    board: Board = []
    for row in rows:
        pile_row: List[Pile] = []
        for letter in row:
            if (
                not isinstance(letter, str)
                or len(letter) != 1
                or not letter.isascii()
                or not letter.isalpha()
            ):
                raise ValueError("each cell must contain one ASCII letter")
            pile_row.append([letter])
        board.append(pile_row)
    return board


def _is_vowel(letter: str) -> bool:
    return letter.casefold() in VOWELS


def _merge_piles(target: Pile, flipped_source: Pile) -> Pile:
    """Place a flipped source pile on a target pile and resolve contact."""
    result = target[:]
    incoming_index = 0

    while result and incoming_index < len(flipped_source):
        target_letter = result[-1]
        incoming_letter = flipped_source[incoming_index]

        if _is_vowel(target_letter) != _is_vowel(incoming_letter):
            result.pop()
            incoming_index += 1
            continue

        if target_letter.casefold() == incoming_letter.casefold():
            result[-1] = target_letter.upper()
            incoming_index += 1

        # A surviving letter remains between the two piles, so the rest of
        # each pile keeps its existing order without further contact reactions.
        break

    return result + flipped_source[incoming_index:]


def _fold_left(board: Board) -> Board:
    height = len(board)
    width = len(board[0])
    new_width = (width + 1) // 2
    result: Board = [
        [board[row][column][:] for column in range(new_width)]
        for row in range(height)
    ]

    for offset in range(width // 2):
        source_column = width - 1 - offset
        target_column = offset
        for row in range(height):
            flipped_source = list(reversed(board[row][source_column]))
            result[row][target_column] = _merge_piles(
                result[row][target_column], flipped_source
            )

    return result


def _fold_right(board: Board) -> Board:
    height = len(board)
    width = len(board[0])
    first_target_column = width // 2
    result: Board = [
        [board[row][column][:] for column in range(first_target_column, width)]
        for row in range(height)
    ]

    for source_column in range(width // 2):
        target_column = width - 1 - source_column
        result_column = target_column - first_target_column
        for row in range(height):
            flipped_source = list(reversed(board[row][source_column]))
            result[row][result_column] = _merge_piles(
                result[row][result_column], flipped_source
            )

    return result


def _fold_up(board: Board) -> Board:
    height = len(board)
    width = len(board[0])
    new_height = (height + 1) // 2
    result: Board = [
        [board[row][column][:] for column in range(width)]
        for row in range(new_height)
    ]

    for offset in range(height // 2):
        source_row = height - 1 - offset
        target_row = offset
        for column in range(width):
            flipped_source = list(reversed(board[source_row][column]))
            result[target_row][column] = _merge_piles(
                result[target_row][column], flipped_source
            )

    return result


def _fold_down(board: Board) -> Board:
    height = len(board)
    width = len(board[0])
    first_target_row = height // 2
    result: Board = [
        [board[row][column][:] for column in range(width)]
        for row in range(first_target_row, height)
    ]

    for source_row in range(height // 2):
        target_row = height - 1 - source_row
        result_row = target_row - first_target_row
        for column in range(width):
            flipped_source = list(reversed(board[source_row][column]))
            result[result_row][column] = _merge_piles(
                result[result_row][column], flipped_source
            )

    return result


def squeeze_matrix(
    matrix: Sequence[Sequence[str]],
    folds: Iterable[str],
) -> List[List[str]]:
    """Apply left/right/up/down folds and return the final top-view grid.

    A fold leaves the crease row or column in place when that dimension is odd.
    Cell piles are returned as their top letter; an empty pile is shown as "_".

    Directions are case-insensitive. The input matrix is not modified.
    """
    board = _make_board(matrix)
    fold_functions = {
        "left": _fold_left,
        "right": _fold_right,
        "up": _fold_up,
        "down": _fold_down,
    }

    for direction in folds:
        if not isinstance(direction, str):
            raise ValueError("each fold direction must be a string")
        normalized = direction.strip().lower()
        if normalized not in DIRECTIONS:
            raise ValueError(
                "fold direction must be one of: left, right, up, down"
            )
        board = fold_functions[normalized](board)

    return [
        [pile[-1] if pile else "_" for pile in row]
        for row in board
    ]
