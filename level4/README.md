# Matrix squeezer

`squeeze_matrix(matrix, folds)` implements `Task.md` in Python 3.7+ using only
the standard library. Run this example from the repository root:

```python
from level4.matrix_squeezer import squeeze_matrix

print(squeeze_matrix(["bcdf"], ["left"]))          # ['fd']
print(squeeze_matrix(["bcdf"], ["left", "left"])) # ['c']
print(squeeze_matrix(["ab"], ["left"]))           # ['_']
print(squeeze_matrix(["bb"], ["left"]))           # ['B']
```

Supply a rectangular sequence of strings or rows of single ASCII letters, and
an iterable of directions: `left`, `right`, `up`, `down`. The function returns a
list of strings showing the final top view without modifying the input.

- A direction names the stationary side: `left` flips the right half onto the
  left half; `up` flips the bottom half onto the top half.
- For odd dimensions the middle row or column is retained as the crease and
  is not flipped. Each folded dimension becomes its previous size rounded up
  after dividing by two. A dimension of one remains unchanged.
- Each cell retains its complete stack between folds. Moving stacks reverse
  their bottom-to-top order and land on top of stationary stacks.
- Reactions occur at newly formed contact surfaces, including contacts exposed
  by a reaction. Identical letters fuse to one uppercase letter; otherwise a
  vowel/consonant pair disappears. Case is ignored when comparing letters, and
  vowels are `a`, `e`, `i`, `o`, `u` (`y` is a consonant).
- Nonreacting letters stay stacked. Empty cells display `_`; underscores are
  output markers, not valid input letters. An empty grid returns `[]`.
- Invalid shapes, letters, or direction names raise `ValueError`. Non-string
  cells/directions or a bare string supplied as `folds` raise `TypeError`.

Time is O(RC(F + 1)) in the worst case for R original rows, C original columns,
and F folds: each fold may traverse surviving letters. Space is O(RC + F),
including materialized directions and full letter stacks.

Run the tests from the repository root:

```sh
python3 -m unittest discover -s level4 -v
```
