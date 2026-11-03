def squeeze_matrix(matrix, folds):
    """Fold a rectangular letter grid and return its top view.

    Directions fold the far side onto the named side:
    left = right onto left; up = bottom onto top.
    Empty piles appear as "_".
    """
    vowels = set("aeiou")

    def as_pile(cell):
        if cell is None or cell == "_" or cell == "":
            return []
        pile = list(cell) if isinstance(cell, str) else list(cell)
        if any(not isinstance(ch, str) or len(ch) != 1 for ch in pile):
            raise ValueError("Pile contents must be single characters")
        return pile

    def merge(target, incoming):
        # The incoming pile flips before landing on the target pile.
        moving = list(reversed(incoming))
        i, j = len(moving), 0

        # The contact seam is moving[i - 1] against target[j].
        while i > 0 and j < len(target):
            a, b = moving[i - 1], target[j]
            a_lower, b_lower = a.casefold(), b.casefold()
            a_vowel = a_lower in vowels
            b_vowel = b_lower in vowels
            a_consonant = a.isalpha() and not a_vowel
            b_consonant = b.isalpha() and not b_vowel

            if (a_vowel and b_consonant) or (b_vowel and a_consonant):
                # Both letters disappear; the next pair becomes the seam.
                i -= 1
                j += 1
                continue

            if a_lower == b_lower:
                # They fuse into one capitalized letter.
                return moving[:i - 1] + [a.upper()] + target[j + 1:]

            break

        return moving[:i] + target[j:]

    board = [[as_pile(cell) for cell in row] for row in matrix]
    rows = len(board)
    cols = len(board[0]) if rows else 0

    if any(len(row) != cols for row in board):
        raise ValueError("The matrix must be rectangular")

    if isinstance(folds, str):
        folds = [folds]

    for direction in folds:
        direction = direction.lower()

        if direction == "left":
            new_cols = (cols + 1) // 2
            for r in range(rows):
                for c in range(new_cols):
                    source = cols - 1 - c
                    if source != c:  # the odd-width crease stays put
                        board[r][c] = merge(board[r][c], board[r][source])
            board = [row[:new_cols] for row in board]
            cols = new_cols

        elif direction == "right":
            start = cols // 2
            for r in range(rows):
                for c in range(start, cols):
                    source = cols - 1 - c
                    if source != c:
                        board[r][c] = merge(board[r][c], board[r][source])
            board = [row[start:] for row in board]
            cols -= start

        elif direction == "up":
            new_rows = (rows + 1) // 2
            for r in range(new_rows):
                source = rows - 1 - r
                if source != r:  # the odd-height crease stays put
                    for c in range(cols):
                        board[r][c] = merge(board[r][c], board[source][c])
            board = board[:new_rows]
            rows = new_rows

        elif direction == "down":
            start = rows // 2
            for r in range(start, rows):
                source = rows - 1 - r
                if source != r:
                    for c in range(cols):
                        board[r][c] = merge(board[r][c], board[source][c])
            board = board[start:]
            rows -= start

        else:
            raise ValueError(f"Unknown fold direction: {direction!r}")

    return [[pile[0] if pile else "_" for pile in row] for row in board]
