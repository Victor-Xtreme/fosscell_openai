# Level 4: Matrix Squeezer

`matrix_squeezer.py` provides `squeeze_matrix(matrix, folds)`, which folds a
rectangular grid of ASCII letters and returns the final top view as a list of
strings. A cell that has no letters left is shown as `_`.

```python
from level4.matrix_squeezer import squeeze_matrix

print(squeeze_matrix(["abC", "DeF"], ["left", "down"]))
```

Directions are `left`, `right`, `up`, and `down`. A fold moves the far half
onto the stationary half, reflecting cells across the fold line. For odd
dimensions, the center row or column remains stationary. The folded dimension
becomes its ceiling half, so repeated folds can be applied in order.

Each cell is a pile stored from bottom to top. A moving pile reverses when it
flips and is stacked on the stationary pile. At the contact seam, identical
letters (case-insensitively) fuse into one uppercase letter; a vowel meeting a
consonant removes both. Reactions continue at the seam while the next pair
reacts.

The input must be a non-empty rectangular grid of single ASCII letters.
Invalid cells or directions raise `ValueError`. With `R*C` cells and `P`
letters in their piles, each fold takes O(R*C + P) time and O(R*C + P) space.
