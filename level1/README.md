# DataProcessor

## Overview

`data_processor.py` provides `DataProcessor` for numerical dataset ingestion,
statistical analysis, filtering, linear transformations, and CSV persistence.
Datasets are stored in memory as floats. The module has no command-line interface.

## Requirements & setup

Python **3.7+** is required. Only the standard library is used: `math`, `csv`,
and `typing`. No installation or third-party dependencies are needed.

From the repository root, import with:

```python
from level1.data_processor import DataProcessor
```

Alternatively, from the `level1` directory use
`from data_processor import DataProcessor`. Running
`python3 level1/data_processor.py` loads the definitions and exits without output.

## API reference

`Number` means `int` or `float`, excluding `bool`. All numbers must be finite and
representable as floats. Invalid types raise `TypeError`; non-finite or
unrepresentable values raise `ValueError`. These rules apply to data, filter
bounds, and transformation parameters. Methods returning `None` mutate data or
write a file as described below.

| Signature | Parameters | Return value | Exceptions |
| --- | --- | --- | --- |
| `DataProcessor(initial_data: Optional[Iterable[Number]] = None)` | Optional iterable of initial numbers; copied into the dataset. | New `DataProcessor`; defaults to empty. | `TypeError` for a non-iterable or invalid items; `ValueError` for invalid numeric values. |
| `add_data(item: Number) -> None` | One number to append. | `None`. | Numeric validation errors. |
| `batch_add(items: Iterable[Number]) -> None` | Iterable of numbers to append. | `None`; validates the whole batch before appending. | `TypeError` for non-iterables or invalid items; `ValueError` for invalid numeric values. Exceptions from input iterators propagate. |
| `calculate_mean() -> float` | No parameters. | Arithmetic mean. | `ValueError` for empty data. |
| `calculate_median() -> float` | No parameters. | Middle sorted value, or mean of the two middle values. | `ValueError` for empty data. |
| `calculate_variance() -> float` | No parameters. | Population variance, dividing by N. | `ValueError` for empty data; `OverflowError` for extreme calculations. |
| `calculate_std_dev() -> float` | No parameters. | Square root of population variance. | Same as variance. |
| `get_summary_statistics() -> Dict[str, Union[int, float]]` | No parameters. | Keys: `count` (int), `mean`, `median`, `variance`, `std_dev`, `min`, `max` (floats). | `ValueError` for empty data; `OverflowError` for extreme calculations. |
| `filter_data(min_val: Optional[Number] = None, max_val: Optional[Number] = None) -> List[float]` | Inclusive lower/upper bounds; `None` means unbounded. | New list of matching values in input order; leaves stored data unchanged. | Numeric validation errors; `ValueError` if minimum exceeds maximum. |
| `transform_data(factor: Number = 1, offset: Number = 0) -> None` | Scale and shift for `value * factor + offset`. | `None`; replaces stored values after validating all results. | Numeric validation errors, including non-finite results. |
| `export_to_csv(filepath: str) -> None` | Output file path; path-like objects also work. | `None`; writes UTF-8 CSV with a single `value` header and one number per row. | `OSError` subclasses for file failures; `TypeError` for invalid path types. |
| `load_from_csv(filepath: str) -> None` | Input path in the exported format; path-like objects also work. | `None`; replaces existing data after validating the complete file. | `ValueError` for invalid header, row shape, or numbers; `csv.Error` for malformed CSV; `UnicodeDecodeError` for invalid UTF-8; `OSError` for file failures; `TypeError` for invalid path types. |

## Runnable examples

Run this example from the repository root:

```python
from pathlib import Path
from tempfile import TemporaryDirectory
from level1.data_processor import DataProcessor

processor = DataProcessor([1, 2, 3])
processor.add_data(4)
processor.batch_add([5, 6])
summary = processor.get_summary_statistics()
assert summary["count"] == 6
assert summary["mean"] == 3.5
assert summary["median"] == 3.5
print(summary)

# Filtering returns a separate list. Construct a new processor to retain it.
filtered = DataProcessor(processor.filter_data(min_val=2, max_val=4))
filtered.transform_data(factor=2, offset=1)
assert filtered.filter_data() == [5.0, 7.0, 9.0]

with TemporaryDirectory() as directory:
    filepath = Path(directory) / "numbers.csv"
    filtered.export_to_csv(filepath)
    restored = DataProcessor()
    restored.load_from_csv(filepath)
    assert restored.get_summary_statistics() == filtered.get_summary_statistics()
```

## Error handling & edge cases

- Empty datasets support additions, filtering, transformations, and CSV export.
  Statistical methods raise `ValueError`. A singleton has zero population variance
  and standard deviation.
- Strings, booleans, complex numbers, and other non-numeric types are rejected;
  CSV numeric text is explicitly parsed on load. NaN and infinities are rejected.
- Failed batches, transformations, or CSV loads leave existing data unchanged.
  Duplicate values and input order are preserved; median calculation does not
  reorder stored data.
- CSV requires exactly the `value` header and one finite number per subsequent
  row. A header-only file loads an empty dataset; a zero-byte file is invalid.
- Export overwrites an existing file and does not create parent directories.
  File errors propagate; writes are not atomic and may leave partial output.
- Values use floating-point arithmetic, so large integers can lose precision.
  Extreme variance calculations can overflow or produce infinity even when
  individual inputs are finite. This is an in-memory utility, not an arbitrary
  precision or streaming statistics engine.
