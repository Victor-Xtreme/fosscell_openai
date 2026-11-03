"""Run from the repository root with python3 -m unittest discover -s level4."""

import unittest

from matrix_squeezer import squeeze_matrix


class MatrixSqueezerTests(unittest.TestCase):
    def test_direction_and_mirroring(self):
        self.assertEqual(squeeze_matrix(["bcdf"], ["left"]), ["fd"])
        self.assertEqual(squeeze_matrix(["bcdf"], ["right"]), ["cb"])
        self.assertEqual(squeeze_matrix(["b", "c", "d", "f"], ["up"]), ["f", "d"])
        self.assertEqual(squeeze_matrix(["b", "c", "d", "f"], ["down"]), ["c", "b"])

    def test_odd_crease_stays_still(self):
        self.assertEqual(squeeze_matrix(["bcdfg"], ["left"]), ["gfd"])
        self.assertEqual(squeeze_matrix(["bcdfg"], ["right"]), ["dcb"])
        self.assertEqual(squeeze_matrix(["b", "c", "d"], ["up"]), ["d", "c"])
        self.assertEqual(squeeze_matrix(["b", "c", "d"], ["down"]), ["c", "b"])

    def test_letter_reactions(self):
        for row in ("ab", "ba", "Ab", "bA"):
            self.assertEqual(squeeze_matrix([row], ["left"]), ["_"])
        for row in ("bb", "bB", "BB", "aa", "aA"):
            self.assertEqual(squeeze_matrix([row], ["left"]), [row[0].upper()])
        self.assertEqual(squeeze_matrix(["ae"], ["left"]), ["e"])

    def test_stack_is_reversed_on_second_fold(self):
        # First fold: [b,f], [c,d]. Second fold moves [d,c] onto [b,f].
        self.assertEqual(squeeze_matrix(["bcdf"], ["left", "left"]), ["c"])
        self.assertEqual(squeeze_matrix(["bcdf"], ["right", "right"]), ["d"])

    def test_cascading_reactions(self):
        # First fold gives [b,b->B], [c,a->empty], then B falls onto empty.
        self.assertEqual(squeeze_matrix(["bcab"], ["left", "left"]), ["B"])
        # First fold gives [b,f], [c,d] in the top row and [e,a]
        # in each bottom cell. The vertical fold cancels all four stacks.
        self.assertEqual(squeeze_matrix(["bcdf", "eeaa"], ["left", "up"]), ["__"])

    def test_empty_singleton_and_no_folds(self):
        self.assertEqual(squeeze_matrix([], ["left"]), [])
        self.assertEqual(squeeze_matrix(["a"], ["left", "right", "up", "down"]), ["a"])
        matrix = [list("ab"), list("cd")]
        self.assertEqual(squeeze_matrix(matrix, []), ["ab", "cd"])
        squeeze_matrix(matrix, ["left", "up"])
        self.assertEqual(matrix, [list("ab"), list("cd")])

    def test_validation(self):
        for matrix in ([""], ["ab", "c"], ["a1"], ["é"], ["_"]):
            with self.assertRaises(ValueError):
                squeeze_matrix(matrix, [])
        with self.assertRaises(TypeError):
            squeeze_matrix([[1]], [])
        with self.assertRaises(ValueError):
            squeeze_matrix(["ab"], ["sideways"])
        with self.assertRaises(TypeError):
            squeeze_matrix(["ab"], "left")
        with self.assertRaises(TypeError):
            squeeze_matrix(["ab"], [1])


if __name__ == "__main__":
    unittest.main()
