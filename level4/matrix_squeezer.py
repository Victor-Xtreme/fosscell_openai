"""Fold a letter grid, combining bottom-to-top letter stacks at each cell."""

from typing import Iterable, List, Sequence

VOWELS = frozenset("aeiou")
DIRECTIONS = frozenset(("left", "up", "right", "down"))


def _combine(target: List[str], incoming: List[str]) -> None:
    """Flip incoming onto target and resolve reactions at the contact surface."""
    for letter in reversed(incoming):
        while target:
            top = target[-1]
            if top.lower() == letter.lower():
                letter = target.pop().upper()
            elif (top.lower() in VOWELS) != (letter.lower() in VOWELS):
                target.pop()
                break
            else:
                target.append(letter)
                break
        else:
            target.append(letter)


def squeeze_matrix(matrix: Sequence[Sequence[str]],
                   folds: Iterable[str]) -> List[str]:
    """Return the top view after folding a rectangular ASCII letter grid.

    Each cell starts as a one-letter stack. Directions name the stationary side:
    left folds right onto left; up folds bottom onto top (and vice versa).
    An odd middle row/column is retained without flipping. Incoming stacks are
    reversed and placed atop stationary stacks. At their contact surface,
    identical letters fuse to uppercase; vowel/consonant pairs disappear.
    Reactions cascade through newly exposed contacts, ignoring letter case.

    Empty cells render as '_'. The input is not mutated. An empty grid returns
    [], but zero-width or ragged rows are invalid. Folding a dimension of one
    leaves it unchanged. Invalid grids or direction names raise ValueError;
    non-string cells/directions raise TypeError. A bare direction string is
    rejected: supply a list such as ['left'] instead.
    """
    if isinstance(folds, str):
        raise TypeError("folds must be an iterable of direction names.")
    directions = list(folds)
    for direction in directions:
        if not isinstance(direction, str):
            raise TypeError("Fold directions must be strings.")
        if direction not in DIRECTIONS:
            raise ValueError("Unknown fold direction: {!r}".format(direction))

    rows = list(matrix)
    if not rows:
        return []
    width = len(rows[0])
    if width == 0 or any(len(row) != width for row in rows):
        raise ValueError("The matrix must be rectangular with nonempty rows.")
    board = []
    for row in rows:
        stacks = []
        for letter in row:
            if not isinstance(letter, str):
                raise TypeError("Each matrix cell must be a letter string.")
            if len(letter) != 1 or not ("a" <= letter <= "z" or "A" <= letter <= "Z"):
                raise ValueError("Each matrix cell must be one ASCII letter.")
            stacks.append([letter])
        board.append(stacks)

    for direction in directions:
        # Transpose vertical folds into horizontal ones to share folding logic.
        vertical = direction in ("up", "down")
        if vertical:
            board = [list(column) for column in zip(*board)]
        width = len(board[0])
        half = width // 2
        retained = width - half
        toward_start = direction in ("left", "up")
        folded = []
        for row in board:
            if toward_start:
                stationary = row[:retained]
                for source in range(retained, width):
                    _combine(stationary[width - 1 - source], row[source])
            else:
                stationary = row[half:]
                for source in range(half):
                    _combine(stationary[width - 1 - source - half], row[source])
            folded.append(stationary)
        board = folded
        if vertical:
            board = [list(column) for column in zip(*board)]

    return ["".join(stack[-1] if stack else "_" for stack in row)
            for row in board]
