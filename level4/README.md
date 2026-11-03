# Matrix squeezer

`fold_matrix(matrix, directions)` folds a rectangular letter grid and returns
its top view as a list of strings. Requires Python 3.7+; no external packages.
Run examples from `level4`:

```python
from matrix_squeezer import fold_matrix

assert fold_matrix(["bc", "df"], ["left"]) == ["c", "f"]
assert fold_matrix(["bcdef"], ["left"]) == ["f_d"]
assert fold_matrix(["baec"], ["left", "left"]) == ["_"]
```

## Folding rules

- `left`: right half flips onto left; `right`: left flips onto right.
- `up`: bottom half flips onto top; `down`: top flips onto bottom.
- An odd middle row or column remains stationary and is retained. Folding
  halves the relevant dimension, rounded up. A dimension of one stays intact.
- Every cell holds a pile ordered from bottom to top. Moving piles reverse
  their layer order before landing on stationary piles.
- Arriving letters interact with the exposed top, in landing order. A vowel
  meeting a consonant removes both. Identical letters fuse into one uppercase
  letter. All other contacts stack. Matching ignores case; vowels are A/E/I/O/U.
- Contacts resolve repeatedly, including after annihilation exposes a lower
  letter. Fused letters participate in subsequent collisions normally.
- Empty piles remain valid cells and display as `_`. Hidden layers survive
  between folds; only the final result is flattened to a top view.

The task leaves case matching and empty-cell symbols open; this implementation
uses the explicit conventions above. Input cells must be individual ASCII
letters, with no initial empty piles. Rows can be strings or iterables of
single-character strings. Directions are case-insensitive strings supplied in
an iterable, such as `["left", "up"]`. Inputs are not modified.

Empty or ragged grids, invalid letters, and unknown directions raise
`ValueError`. Non-iterable inputs or a bare direction string raise `TypeError`.

## Tests and complexity

```sh
python -B -m unittest discover -s level4 -v
```

Run that command from the repository root. Tests cover all directions, odd
creases, reversed stacks, collisions, cascades, empty piles, and validation.

For N initial letters and F folds, the implementation takes O(N(F + 1)) time
in the worst case and O(N + F) auxiliary space. Each fold moves or visits at
most O(N) cells/layers; each collision removes at least one letter. Directions
are materialized once for validation before processing.
