"""Compute Fibonacci numbers in linear time and constant auxiliary space."""


def fibonacci(n: int) -> int:
    """Return F(n), where F(0) = 0 and F(1) = 1.

    Runs in O(n) time and O(1) extra space, avoiding the repeated work of the
    naive recursive definition (which takes exponential time).
    """
    if not isinstance(n, int) or isinstance(n, bool):
        raise TypeError("n must be an integer")
    if n < 0:
        raise ValueError("n must be non-negative")

    previous, current = 0, 1
    for _ in range(n):
        previous, current = current, previous + current
    return previous


if __name__ == "__main__":
    try:
        value = int(input("Enter a non-negative integer: "))
        print(f"F({value}) = {fibonacci(value)}")
    except (ValueError, TypeError) as error:
        raise SystemExit(str(error)) from error
