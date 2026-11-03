import unittest

from matrix_squeezer import fold_matrix


class MatrixFoldingTests(unittest.TestCase):
    def test_all_directions(self):
        grid = ["bc", "df"]
        self.assertEqual(fold_matrix(grid, ["left"]), ["c", "f"])
        self.assertEqual(fold_matrix(grid, ["right"]), ["b", "d"])
        self.assertEqual(fold_matrix(grid, ["up"]), ["df"])
        self.assertEqual(fold_matrix(grid, ["down"]), ["bc"])

    def test_odd_creases(self):
        self.assertEqual(fold_matrix(["bcdef"], ["left"]), ["f_d"])
        self.assertEqual(fold_matrix(["bcdef"], ["right"]), ["d_b"])
        self.assertEqual(fold_matrix(["b", "c", "d"], ["up"]), ["d", "c"])
        self.assertEqual(fold_matrix(["b", "c", "d"], ["down"]), ["c", "b"])

    def test_stack_reversal(self):
        self.assertEqual(fold_matrix(["bcdf"], ["left", "right"]), ["b"])
        self.assertEqual(fold_matrix(["b", "c", "d", "f"], ["up", "down"]), ["b"])

    def test_collisions(self):
        for pair in ["ab", "ba", "Ac", "dE"]:
            self.assertEqual(fold_matrix([pair], ["left"]), ["_"])
        for pair in ["aa", "aA", "bb", "Bb"]:
            self.assertEqual(fold_matrix([pair], ["left"]), [pair[0].upper()])
        self.assertEqual(fold_matrix(["ae"], ["left"]), ["e"])
        self.assertEqual(fold_matrix(["by"], ["left"]), ["y"])

    def test_cascade_and_empty_piles(self):
        # First fold leaves [b,c] and [a,e]. Reversal moves e then a:
        # e annihilates c, then a annihilates the exposed b.
        self.assertEqual(fold_matrix(["baec"], ["left", "left"]), ["_"])
        self.assertEqual(fold_matrix(["aaAA"], ["left", "left"]), ["A"])
        self.assertEqual(fold_matrix(["abba"], ["left", "right"]), ["_"])
        self.assertEqual(fold_matrix(["abcd"], ["left", "left"]), ["b"])

    def test_mixed_axes(self):
        self.assertEqual(fold_matrix(["bc", "df"], ["left", "up"]), ["d"])
        self.assertEqual(fold_matrix(["bc", "df"], ["left", "down"]), ["b"])

    def test_no_folds_single_axis_and_input_preservation(self):
        grid = [["a", "B", "c"]]
        self.assertEqual(fold_matrix(grid, []), ["aBc"])
        self.assertEqual(fold_matrix(grid, ["up", "down"]), ["aBc"])
        self.assertEqual(grid, [["a", "B", "c"]])
        self.assertEqual(fold_matrix(["Z"], ["left", "right", "up", "down"]), ["Z"])
        self.assertEqual(fold_matrix(["bb"], iter(["LEFT"])), ["B"])

    def test_invalid_inputs(self):
        for grid in [[], [""], ["a", "bc"], ["a_"], ["é"], [[1]], [["ab"]]]:
            with self.subTest(grid=grid), self.assertRaises(ValueError):
                fold_matrix(grid, [])
        for directions in [["sideways"], [1]]:
            with self.assertRaises(ValueError):
                fold_matrix(["a"], directions)
        for grid, directions in [(None, []), (["a"], None), (["a"], "left")]:
            with self.assertRaises(TypeError):
                fold_matrix(grid, directions)


if __name__ == "__main__":
    unittest.main()
