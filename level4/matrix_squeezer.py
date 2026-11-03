"""
Level 4: Matrix Squeezer / Paper Folder
"""
from typing import List

VOWELS = set("aeiouAEIOU")

def interact_piles(base_pile: List[str], incoming_pile: List[str]) -> List[str]:
    """
    Simulates the physical contact when an incoming pile flips and lands
    on top of the base pile:
    1. Incoming pile flips upside down (reversed).
    2. Letters land one by one onto the target stack.
    3. The meeting boundary resolves interactions:
       - Vowel + Consonant -> both are removed.
       - Identical letters -> fuse into a single uppercase letter.
       - Otherwise -> placed on top.
    """
    # Physically flipping turns the incoming pile upside down
    incoming_flipped = list(reversed(incoming_pile))

    stack = list(base_pile)
    for char in incoming_flipped:
        if not stack:
            stack.append(char)
            continue

        top = stack[-1]

        # Case 1: Two identical letters fuse into uppercase
        if top.lower() == char.lower():
            stack.pop()
            stack.append(top.upper())

        # Case 2: Vowel + Consonant zap each other out
        elif (top in VOWELS and char not in VOWELS) or (top not in VOWELS and char in VOWELS):
            stack.pop()

        # Case 3: No interaction, sits on top
        else:
            stack.append(char)

    return stack


def fold_left(grid: List[List[List[str]]]) -> List[List[List[str]]]:
    """Folds the right side onto the left side."""
    rows = len(grid)
    cols = len(grid[0])
    new_cols = (cols + 1) // 2
    new_grid = [[[] for _ in range(new_cols)] for _ in range(rows)]

    for r in range(rows):
        for c in range(new_cols):
            target = grid[r][c]
            mirror_c = cols - 1 - c

            if mirror_c != c:
                incoming = grid[r][mirror_c]
                new_grid[r][c] = interact_piles(target, incoming)
            else:
                new_grid[r][c] = target

    return new_grid


def rotate_cw(grid: List[List[List[str]]]) -> List[List[List[str]]]:
    """Rotates grid 90 degrees clockwise."""
    rows = len(grid)
    cols = len(grid[0])
    return [[grid[rows - 1 - r][c] for r in range(rows)] for c in range(cols)]


def rotate_ccw(grid: List[List[List[str]]]) -> List[List[List[str]]]:
    """Rotates grid 90 degrees counter-clockwise."""
    rows = len(grid)
    cols = len(grid[0])
    return [[grid[r][cols - 1 - c] for r in range(rows)] for c in range(cols - 1, -1, -1)]


def fold_grid(grid: List[List[List[str]]], direction: str) -> List[List[List[str]]]:
    d = direction.strip().lower()
    if d == "left":
        return fold_left(grid)
    elif d == "right":
        flipped = [row[::-1] for row in grid]
        folded = fold_left(flipped)
        return [row[::-1] for row in folded]
    elif d == "up":
        rotated = rotate_ccw(grid)
        folded = fold_left(rotated)
        return rotate_cw(folded)
    elif d == "down":
        rotated = rotate_cw(grid)
        folded = fold_left(rotated)
        return rotate_ccw(folded)
    else:
        raise ValueError(f"Invalid fold direction: {direction}")


def matrix_squeezer(board: List[str], folds: List[str]) -> List[str]:
    """
    Main function.
    :param board: Initial 2D grid represented as a list of equal-length strings.
    :param folds: Ordered sequence of fold directions ('left', 'right', 'up', 'down').
    :return: Top-view strings representing the state of the grid.
    """
    if not board or not board[0]:
        return []

    # Initialize grid: each cell is a vertical stack of letters
    grid = [[[char] for char in row] for row in board]

    for f in folds:
        if not grid or not grid[0]:
            break
        grid = fold_grid(grid, f)

    # Render top view
    output = []
    for row in grid:
        line = []
        for pile in row:
            line.append(pile[-1] if pile else "_")
        output.append("".join(line))

    return output


if __name__ == "__main__":
    sample_board = [
        "abcde",
        "fghij",
        "klmno"
    ]
    sample_folds = ["left", "up"]
    result = matrix_squeezer(sample_board, sample_folds)
    for r in result:
        print(r)
