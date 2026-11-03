from collections import deque

VOWELS = set("aeiou")


def _is_zap(a, b):
    """True if exactly one of the two letters is a vowel (both must be letters)."""
    if not (a.isalpha() and b.isalpha()):
        return False
    return (a.lower() in VOWELS) != (b.lower() in VOWELS)


def _merge(target, source):
    """Flip `source` over onto `target`; return the resulting pile."""
    pile = list(target)
    incoming = deque(reversed(source))  # the physical flip

    while pile and incoming:
        t, s = pile[-1], incoming[0]
        if t == s and t.isalpha():          # fuse
            pile[-1] = t.upper()
            incoming.popleft()
        elif _is_zap(t, s):                 # annihilate
            pile.pop()
            incoming.popleft()
        else:
            break

    pile.extend(incoming)
    return pile


def _fold_left(grid):
    """Right side flips onto the left. grid is a list of rows of piles."""
    if not grid:
        return grid
    width = len(grid[0])
    new_width = (width + 1) // 2            # odd: crease column is kept

    folded = []
    for row in grid:
        new_row = [list(row[c]) for c in range(new_width)]
        for c in range(new_width, width):
            dest = width - 1 - c            # mirror across the crease
            new_row[dest] = _merge(new_row[dest], row[c])
        folded.append(new_row)
    return folded


def _transpose(grid):
    return [list(col) for col in zip(*grid)] if grid else grid


def _mirror(grid):
    return [row[::-1] for row in grid]


def fold_matrix(letters, folds, empty="_"):
    """
    letters: list of equal-length strings (or list of lists of single chars)
    folds:   iterable of 'left' | 'right' | 'up' | 'down'
    returns: top-down view as a newline-joined string
    """
    if not letters:
        return ""
    if len({len(r) for r in letters}) != 1:
        raise ValueError("All rows must have the same length")

    grid = [[[ch] for ch in row] for row in letters]  # every cell is a pile

    for direction in folds:
        d = direction.lower()
        if d == "left":
            grid = _fold_left(grid)
        elif d == "right":
            grid = _mirror(_fold_left(_mirror(grid)))
        elif d == "up":
            grid = _transpose(_fold_left(_transpose(grid)))
        elif d == "down":
            grid = _transpose(_mirror(_fold_left(_mirror(_transpose(grid)))))
        else:
            raise ValueError(f"Unknown fold direction: {direction!r}")

    return "\n".join(
        "".join(pile[-1] if pile else empty for pile in row) for row in grid
    )


if __name__ == "__main__":
    print(fold_matrix(["abc", "def"], ["left"]))
    # _b
    # fe
