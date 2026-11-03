"""Compute Fibonacci numbers without exponential recursive recomputation."""


def fibonacci(n: int) -> int:
    """Return F(n), with F(0)=0 and F(1)=1.

    Uses O(n) additions and a constant number of integer variables.
    Python's growing integers require O(n) bits and O(n**2) bit work.
    """
    if not isinstance(n, int) or isinstance(n, bool):
        raise TypeError("n must be an integer")
    if n < 0:
        raise ValueError("n must be non-negative")

    current, following = 0, 1
    for _ in range(n):
        current, following = following, current + following
    return current


if __name__ == "__main__":
    print(fibonacci(10))
