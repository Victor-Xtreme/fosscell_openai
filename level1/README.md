# DataProcessor
`DataProcessor` stores numerical data and provides statistics, filtering, transformations, and CSV import/export.


## Requirements and setup
Requires Python 3.7 or later. It uses only the standard library: `math`, `csv`, and `typing`. No packages need to be installed.
Keep `data_processor.py` in your project, then import the class:
```python
from data_processor import DataProcessor
```



## API reference

### `DataProcessor(initial_data=None)`
Create a processor, optionally initialized with a list of integers or floats.

### `add_data(item)`
Add one numeric value, stored as a float.
- Raises `ValueError` if `item` is `None`.
- Raises `TypeError` if `item` is not an integer or float.

### `batch_add(items)`
Try to add each item in a list. Invalid items are skipped. Returns the number successfully added.

### `get_data()`
Return a copy of the stored values as a list of floats.

### `clear()`
Remove all stored values.

### `calculate_mean()`
Return the arithmetic mean. Raises `ValueError` if the dataset is empty.

### `calculate_median()`
return the median. For an even number of values, it averages the two middle values. Raises `ValueError` if the dataset is empty.

### `calculate_variance()`
Return the sample variance. Raises `ValueError` if there are fewer than two values.

### `calculate_std_dev()`
Return the sample standard deviation. Requires at least two values.

### `get_summary_statistics()`
Return a dictionary of summary statistics. Call this only when the dataset is nonempty, because the mean and median calculations raise `ValueError` for empty data.

### `filter_data(min_val=None, max_val=None)`
Return values within the inclusive range. Either bound can be omitted. The stored data is unchanged.

### `transform_data(factor=1.0, offset=0.0)`
Return a new list with `value * factor + offset` applied to each stored value. The stored data is unchanged.

### `export_to_csv(filepath)`
Write the dataset to a single-column CSV file with the header `value`.

### `load_from_csv(filepath)`
Load numeric values from the `value` column, or the first column if there is no `value` column. Invalid rows are skipped. Values are added to the current dataset. Returns the number added.

File operations can raise Python I/O errors.



## Example
```python
processor = DataProcessor([10, 20, 30])
processor.batch_add([40, 50])

print(processor.get_data())
print(processor.calculate_mean())
print(processor.filter_data(min_val=20, max_val=40))
print(processor.transform_data(factor=2, offset=5))
```

## CSV example
```python
processor.export_to_csv("numbers.csv")

another = DataProcessor()
added = another.load_from_csv("numbers.csv")
print(f"Loaded {added} values:", another.get_data())
```


## Edge cases
- Mean and median require at least one value.
- Sample variance and standard deviation require at least two values.
- `batch_add` skips invalid values and reports how many were added.
- CSV loading skips rows that cannot be converted to numbers.
