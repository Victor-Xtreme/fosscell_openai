```python
"""Fold a grid of letter piles like paper and show the top view."""

VOWELS = set("aeiou")
EMPTY_MARK = "_"


def _is_vowel(ch):
    return ch.lower() in VOWELS


def _fuses(a, b):
    """Identical letters (case-insensitive) fuse into a capital."""
    return a.lower() == b.lower()


def _zaps(a, b):
    """A vowel meeting a consonant destroys both."""
    return _is_vowel(a) != _is_vowel(b)


def _land(target, moving):
    """Flip `moving` upside down and set it on top of `target`.

    Piles are lists ordered bottom -> top. Flipping reverses the order, so
    the old top of `moving` becomes its bottom and touches the top of
    `target`. Only that contact point interacts; each time an interaction
    happens, the next two letters touch and may interact too (chain).
    """
    flipped = moving[::-1]
    pile = list(target)
    i = 0
    while pile and i < len(flipped):
        top, incoming = pile[-1], flipped[i]
        if _fuses(top, incoming):
            pile[-1] = top.upper()
        elif _zaps(top, incoming):
            pile.pop()
        else:
            break
        i += 1
    pile.extend(flipped[i:])
    return pile


def _fold_left(g):
    """Right side flips onto the left. An odd middle column is the crease."""
    w = len(g[0])
    keep = (w + 1) // 2
    out = []
    for row in g:
        new_row = [list(p) for p in row[:keep]]
        for c in range(keep, w):
            t = w - 1 - c
            new_row[t] = _land(new_row[t], row[c])
        out.append(new_row)
    return out


def _mirror(g):
    return [row[::-1] for row in g]


def _transpose(g):
    return [list(col) for col in zip(*g)]


def fold_grid(grid, folds):
    """Fold `grid` (list of equal-length strings or lists of letters).

    `folds` is a list of "left" / "up" / "right" / "down" (case-insensitive).
    Returns the top view as a list of strings; fully wiped piles show "_".
    """
    if not grid or not grid[0]:
        return []

    g = [[[ch] for ch in row] for row in grid]  # every cell is a pile

    for d in folds:
        d = d.strip().lower()
        if d == "left":
            g = _fold_left(g)
        elif d == "right":
            g = _mirror(_fold_left(_mirror(g)))
        elif d == "up":
            g = _transpose(_fold_left(_transpose(g)))
        elif d == "down":
            g = _transpose(_mirror(_fold_left(_mirror(_transpose(g)))))
        else:
            raise ValueError(f"Unknown fold direction: {d!r}")

    return ["".join(p[-1] if p else EMPTY_MARK for p in row) for row in g]


if __name__ == "__main__":
    print("\n".join(fold_grid(["abc", "def"], ["left"])))  # _b / fe
```
