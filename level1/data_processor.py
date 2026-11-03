"""Ingest, analyze, transform, and persist finite numerical datasets."""

import csv
import math
from typing import Dict, Iterable, List, Optional, Union

Number = Union[int, float]


class DataProcessor:
    """A mutable numerical dataset with population statistics.

    Input batches and transformations are validated before changing the dataset.
    Booleans, non-numeric values, and non-finite numbers are rejected.
    """

    def __init__(self, initial_data: Optional[Iterable[Number]] = None) -> None:
        self._data: List[float] = []
        if initial_data is not None:
            self.batch_add(initial_data)

    @staticmethod
    def _validate_number(value: Number) -> float:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise TypeError("Expected an int or float (excluding bool).")
        try:
            number = float(value)
        except OverflowError as error:
            raise ValueError("Number is outside the supported float range.") from error
        if not math.isfinite(number):
            raise ValueError("Numbers must be finite.")
        return number

    def _require_data(self) -> None:
        if not self._data:
            raise ValueError("The dataset is empty.")

    def add_data(self, item: Number) -> None:
        """Append one finite number."""
        self._data.append(self._validate_number(item))

    def batch_add(self, items: Iterable[Number]) -> None:
        """Append an iterable of numbers, leaving data unchanged on failure."""
        values = [self._validate_number(item) for item in items]
        self._data.extend(values)

    def calculate_mean(self) -> float:
        """Return the arithmetic mean; raise ValueError for empty data."""
        self._require_data()
        return math.fsum(value / len(self._data) for value in self._data)

    def calculate_median(self) -> float:
        """Return the middle value or mean of the two middle values."""
        self._require_data()
        values = sorted(self._data)
        middle = len(values) // 2
        if len(values) % 2:
            return values[middle]
        return math.fsum((values[middle - 1] / 2, values[middle] / 2))

    def calculate_variance(self) -> float:
        """Return population variance (divide by N, not N - 1)."""
        mean = self.calculate_mean()
        return math.fsum((value - mean) ** 2 / len(self._data)
                         for value in self._data)

    def calculate_std_dev(self) -> float:
        """Return the square root of population variance."""
        return math.sqrt(self.calculate_variance())

    def get_summary_statistics(self) -> Dict[str, Union[int, float]]:
        """Return count, mean, median, variance, std_dev, min, and max."""
        self._require_data()
        variance = self.calculate_variance()
        return {
            "count": len(self._data),
            "mean": self.calculate_mean(),
            "median": self.calculate_median(),
            "variance": variance,
            "std_dev": math.sqrt(variance),
            "min": min(self._data),
            "max": max(self._data),
        }

    def filter_data(self, min_val: Optional[Number] = None,
                    max_val: Optional[Number] = None) -> List[float]:
        """Return values within inclusive bounds without changing the dataset."""
        lower = None if min_val is None else self._validate_number(min_val)
        upper = None if max_val is None else self._validate_number(max_val)
        if lower is not None and upper is not None and lower > upper:
            raise ValueError("min_val must not exceed max_val.")
        return [value for value in self._data
                if (lower is None or value >= lower)
                and (upper is None or value <= upper)]

    def transform_data(self, factor: Number = 1, offset: Number = 0) -> None:
        """Replace each value with value * factor + offset."""
        scale = self._validate_number(factor)
        shift = self._validate_number(offset)
        values = [self._validate_number(value * scale + shift)
                  for value in self._data]
        self._data = values

    def export_to_csv(self, filepath: str) -> None:
        """Write a one-column UTF-8 CSV with a 'value' header."""
        with open(filepath, "w", encoding="utf-8", newline="") as stream:
            writer = csv.writer(stream)
            writer.writerow(["value"])
            writer.writerows([value] for value in self._data)

    def load_from_csv(self, filepath: str) -> None:
        """Replace data from an exported CSV, only after validating every row."""
        values = []
        with open(filepath, encoding="utf-8", newline="") as stream:
            reader = csv.reader(stream, strict=True)
            if next(reader, None) != ["value"]:
                raise ValueError("CSV must have a single 'value' header.")
            for row in reader:
                if len(row) != 1:
                    raise ValueError("Each CSV row must contain one number.")
                values.append(self._validate_number(float(row[0])))
        self._data = values
