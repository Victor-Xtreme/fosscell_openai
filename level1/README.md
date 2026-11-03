# Level 1 Data Processor

`data_processor.py` exposes `DataProcessor`, a small in-memory numeric dataset utility for ingestion, descriptive statistics, range filtering, linear transformations, and CSV persistence. It also retains a command-line request-log summarizer (`main`, `fast_main`, and `reference_main`) for backwards compatibility; that separate workflow is described below.

## Requirements and setup

- Python 3.7 or later.
- Numeric processing uses the standard library modules `math`, `csv`, and `typing`; the retained request-log workflow also uses standard library modules including `os`, `array`, `bisect`, `marshal`, and `gc`.
- No third-party packages are required.

Import from the `level1` directory (or add it to `PYTHONPATH`):

```python
from data_processor import DataProcessor

processor = DataProcessor([1, 2.5, 3])
```

## Numeric API reference

### `DataProcessor(initial_data: Optional[Iterable[Union[int, float]]] = None) -> None`

Creates an empty processor or initializes it with the supplied iterable. Values must be finite integers or floats; booleans are rejected. `TypeError` is raised for non-numeric values and `ValueError` for non-finite floats. An invalid batch is validated before changing the stored data.

### `add_data(item: Union[int, float]) -> None`

Validates and appends one observation. Raises `TypeError` or `ValueError` for invalid values.

### `batch_add(items: Iterable[Union[int, float]]) -> None`

Validates and appends all observations. Raises `TypeError` or `ValueError`; no values are appended if validation fails.

### Statistics

- `calculate_mean() -> float`: arithmetic mean.
- `calculate_median() -> float`: middle value, or the average of the two middle values.
- `calculate_variance() -> float`: population variance (divides by `N`).
- `calculate_std_dev() -> float`: population standard deviation.
- `get_summary_statistics() -> dict`: mapping with `count`, `mean`, `median`, `variance`, `std_dev`, `min`, and `max`.

Each statistics method raises `ValueError` when the dataset is empty.

### Filtering and transformation

- `filter_data(min_val: Optional[Number] = None, max_val: Optional[Number] = None) -> List[Number]`: returns values within inclusive bounds. `None` means unbounded. Raises `TypeError` or `ValueError` for invalid bounds and `ValueError` if the minimum exceeds the maximum. The stored dataset is unchanged.
- `transform_data(factor: Number, offset: Number) -> List[Number]`: replaces every value with `value * factor + offset` and returns a copy of the transformed values. Both parameters must be finite numbers; raises `TypeError` or `ValueError` otherwise.

### CSV persistence

- `export_to_csv(filepath: str) -> None`: writes a UTF-8 CSV with a `value` header and one observation per row. Filesystem failures raise the corresponding `OSError` subclass.
- `load_from_csv(filepath: str) -> None`: reads that format and replaces the current dataset after successful validation. Numeric values are loaded as floats. Malformed rows or non-numeric/non-finite values raise `ValueError`; file access and decoding failures propagate as `OSError` or `UnicodeError`.

## Examples

```python
from data_processor import DataProcessor

processor = DataProcessor([12, 15, 18])
processor.add_data(21)
processor.batch_add([24, 27])

print(processor.get_summary_statistics())
print(processor.filter_data(min_val=18, max_val=24))
print(processor.transform_data(factor=2, offset=1))

processor.export_to_csv("measurements.csv")
restored = DataProcessor()
restored.load_from_csv("measurements.csv")
print(restored.data)
```

## Error handling and edge cases

- An empty dataset is valid for ingestion, filtering, transformation, and CSV export. Statistical calculations raise `ValueError` because those statistics are undefined without observations.
- Strings, complex numbers, booleans, and other non-numeric objects are rejected with `TypeError`. NaN and infinite floats are rejected with `ValueError`.
- Filtering uses inclusive limits and returns a new list without modifying stored data. Transformations update the stored data.
- CSV loading is atomic: parsing and validation complete before the existing dataset is replaced. The file must have one numeric value per row; the optional header is `value`.

## Retained request-log command line

For compatibility, running `python data_processor.py [--data DIR] [--out DIR]` processes `DIR/requests.log` and writes `DIR/summary.txt` (defaults: `/data` and `/out`). Each input line contains `timestamp method endpoint status latency_us`; timestamps must include fractional seconds and a timezone. The output groups requests by hour and endpoint, reporting count, nearest-rank p50/p95/p99 latency, 5xx count, and errors per million. The module-level `main(data_dir, out_dir)`, `fast_main(data_dir, out_dir, single=False)`, and `reference_main(data_dir, out_dir)` implement this separate workflow.

From this directory, a sample run is:

```bash
python data_processor.py --data ./sample_data --out ./sample_output
cat ./sample_output/summary.txt
```

`A_INGEST_WORKERS` can override worker selection, and `A_INGEST_CHUNK` controls the fast parser's byte chunk size. The parallel implementation requires Unix process and positional-read support (`os.fork` and `os.pread`).
