"""Iterative Fibonacci numbers with constant auxiliary space."""


def fibonacci(n: int) -> int:
    """Return the nth Fibonacci number, where F(0) = 0 and F(1) = 1.

    Raises:
        TypeError: if n is not an integer (booleans are not accepted).
        ValueError: if n is negative.
    """
    if isinstance(n, bool) or not isinstance(n, int):
        raise TypeError("n must be an integer")
    if n < 0:
        raise ValueError("n must be non-negative")

    previous, current = 0, 1
    for _ in range(n):
        previous, current = current, previous + current
    return previous
