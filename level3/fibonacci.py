"""Compute Fibonacci numbers iteratively in linear time and constant space."""


def fibonacci(n: int) -> int:
    """Return the n-th Fibonacci number, with F(0)=0 and F(1)=1.

    Args:
        n: A non-negative integer index.

    Raises:
        TypeError: If n is not an integer (booleans are not accepted).
        ValueError: If n is negative.

    The iterative recurrence takes O(n) additions and O(1) auxiliary space,
    replacing the O(2**n) time of naive recursive recomputation.
    """
    if isinstance(n, bool) or not isinstance(n, int):
        raise TypeError("n must be an integer")
    if n < 0:
        raise ValueError("n must be non-negative")

    previous, current = 0, 1
    for _ in range(n):
        previous, current = current, previous + current
    return previous


if __name__ == "__main__":
    print(fibonacci(10))
