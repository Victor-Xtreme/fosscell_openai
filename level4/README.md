# Level 4: Matrix Squeezer

[`matrix_squeezer.py`](matrix_squeezer.py) provides `squeeze_matrix(grid, folds)`.
It accepts a rectangular grid and a sequence of fold directions (`left`,
`right`, `up`, `down`) and returns a list of strings showing the top letter in
each resulting cell. An empty pile is shown as `_`.

Each cell can be a string or an iterable of letters. Letters in a cell are
ordered bottom-to-top; for example, `"ab"` means `a` is at the bottom and `b`
is on top. When a pile moves in a fold, its order reverses before it lands.
Reactions resolve from the contact seam: a vowel and consonant annihilate, and
matching letters (case-insensitive) fuse into one uppercase letter. On odd
dimensions, the middle row or column remains stationary as the crease.

```python
from matrix_squeezer import squeeze_matrix

grid = [
    ["a", "b", "c", "d"],
    ["e", "f", "g", "h"],
]
print(squeeze_matrix(grid, ["left"]))
```

The module uses only the Python standard library.
