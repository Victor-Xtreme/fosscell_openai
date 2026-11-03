# DataProcessor Documentation

## 1. Overview

The `DataProcessor` module provides a robust, lightweight, and pure-Python toolkit for managing, analyzing, and transforming 1-dimensional numerical datasets. It serves as a self-contained data handling engine designed for applications where external dependencies (such as NumPy or Pandas) are unnecessary or restricted.

### Problems Solved

* **Data Ingestion**: Seamlessly ingest individual scalar numbers or collections of numeric values (`int`, `float`).
* **Statistical Analysis**: Calculate key statistical metrics including arithmetic mean, median, sample/population variance, standard deviation, and aggregated summary statistics.
* **Dataset Filtering**: Query and extract subsets of data based on inclusive numeric boundaries $[min\_val, max\_val]$.
* **Linear Transformations**: Apply scalable linear transformations $y = a \cdot x + b$ across the dataset.
* **CSV Persistence**: Export datasets to standard CSV files and reload previously stored numeric data back into the processor.

---

## 2. Requirements & Setup

### Requirements

* **Python Version**: Python 3.7+
* **Dependencies**: Standard Library only (`math`, `csv`, `typing`). No third-party package installations are required.

### Import and Usage

To import `DataProcessor` in your Python application, ensure `data_processor.py` is in your Python path:

```python
from data_processor import DataProcessor
```

---

## 3. API Reference

### `__init__(initial_data: Optional[List[Union[int, float]]] = None)`

Initializes a new `DataProcessor` instance.

* **Parameters**:
  * `initial_data` (`Optional[List[Union[int, float]]]`, optional): A sequence of numeric elements to populate the internal dataset upon instantiation. Defaults to `None` (empty dataset).
* **Raises**:
  * `TypeError`: If `initial_data` contains non-numeric elements or is not an iterable.

---

### `add_data(item: Union[int, float]) -> None`

Appends a single numeric element to the dataset.

* **Parameters**:
  * `item` (`Union[int, float]`): The numeric value to add.
* **Returns**: `None`
* **Raises**:
  * `TypeError`: If `item` is not an `int` or `float`.

---

### `batch_add(items: List[Union[int, float]]) -> None`

Appends a collection of numeric values to the existing dataset.

* **Parameters**:
  * `items` (`List[Union[int, float]]`): A list or sequence of numeric values.
* **Returns**: `None`
* **Raises**:
  * `TypeError`: If `items` is not iterable or contains non-numeric values.

---

### `calculate_mean() -> float`

Calculates the arithmetic mean ($\mu$) of the current dataset:

$$\mu = \frac{1}{n} \sum_{i=1}^{n} x_i$$

* **Parameters**: None
* **Returns**: `float` - The arithmetic mean of the dataset.
* **Raises**:
  * `ValueError`: If the dataset is empty.

---

### `calculate_median() -> float`

Calculates the median value of the current dataset. If the number of items $n$ is odd, returns the middle element; if $n$ is even, returns the average of the two central values.

* **Parameters**: None
* **Returns**: `float` - The median of the dataset.
* **Raises**:
  * `ValueError`: If the dataset is empty.

---

### `calculate_variance(sample: bool = False) -> float`

Computes the variance of the dataset.

* Population Variance ($\sigma^2$ when `sample=False`):

$$\sigma^2 = \frac{1}{n} \sum_{i=1}^{n} (x_i - \mu)^2$$

* Sample Variance ($s^2$ when `sample=True`):

$$s^2 = \frac{1}{n - 1} \sum_{i=1}^{n} (x_i - \mu)^2$$

* **Parameters**:
  * `sample` (`bool`, optional): If `True`, computes sample variance using $n - 1$ degrees of freedom. Defaults to `False`.
* **Returns**: `float` - The computed variance.
* **Raises**:
  * `ValueError`: If the dataset is empty, or if `sample=True` and dataset length $n \le 1$.

---

### `calculate_std_dev(sample: bool = False) -> float`

Calculates the standard deviation ($\sigma$ or $s$) of the dataset, defined as the square root of the variance:

$$\sigma = \sqrt{\sigma^2}$$

* **Parameters**:
  * `sample` (`bool`, optional): If `True`, calculates sample standard deviation. Defaults to `False`.
* **Returns**: `float` - The standard deviation.
* **Raises**:
  * `ValueError`: If the dataset is empty, or if `sample=True` and $n \le 1$.

---

### `get_summary_statistics() -> Dict[str, Union[int, float]]`

Generates a dictionary containing descriptive statistics for the current dataset.

* **Parameters**: None
* **Returns**: `Dict[str, Union[int, float]]` with keys:
  * `"count"` (`int`): Total number of data points $n$.
  * `"mean"` (`float`): Arithmetic mean.
  * `"median"` (`float`): Median value.
  * `"variance"` (`float`): Population variance.
  * `"std_dev"` (`float`): Population standard deviation.
  * `"min"` (`Union[int, float]`): Minimum value in the dataset.
  * `"max"` (`Union[int, float]`): Maximum value in the dataset.
* **Raises**:
  * `ValueError`: If called on an empty dataset.

---

### `filter_data(min_val: Optional[Union[int, float]] = None, max_val: Optional[Union[int, float]] = None) -> List[Union[int, float]]`

Filters dataset elements matching the numeric condition $min\_val \le x \le max\_val$.

* **Parameters**:
  * `min_val` (`Optional[Union[int, float]]`, optional): Inclusive lower limit. If `None`, no lower bound is applied.
  * `max_val` (`Optional[Union[int, float]]`, optional): Inclusive upper limit. If `None`, no upper bound is applied.
* **Returns**: `List[Union[int, float]]` - A list of matching elements.
* **Raises**:
  * `TypeError`: If bounds are provided but are non-numeric.
  * `ValueError`: If both `min_val` and `max_val` are provided and $min\_val > max\_val$.

---

### `transform_data(factor: Union[int, float] = 1.0, offset: Union[int, float] = 0.0) -> List[Union[int, float]]`

Applies a linear transformation $y = a \cdot x + b$ to every element $x$ in the dataset, where $a$ is `factor` and $b$ is `offset`. Updates the internal dataset state in-place and returns the transformed values.

* **Parameters**:
  * `factor` (`Union[int, float]`, optional): Scaling factor $a$. Defaults to `1.0`.
  * `offset` (`Union[int, float]`, optional): Constant shift $b$. Defaults to `0.0`.
* **Returns**: `List[Union[int, float]]` - The transformed dataset.
* **Raises**:
  * `TypeError`: If `factor` or `offset` are non-numeric.

---

### `export_to_csv(filepath: str) -> None`

Exports the current dataset to a single-column CSV file.

* **Parameters**:
  * `filepath` (`str`): Target filesystem path for the CSV output.
* **Returns**: `None`
* **Raises**:
  * `IOError` / `PermissionError`: If the file cannot be created or written to.

---

### `load_from_csv(filepath: str) -> None`

Reads numbers from a CSV file and loads them into the internal dataset, overwriting existing data.

* **Parameters**:
  * `filepath` (`str`): File path of the source CSV file.
* **Returns**: `None`
* **Raises**:
  * `FileNotFoundError`: If the specified file does not exist.
  * `ValueError`: If the CSV contains non-numeric data or invalid formatting.

---

## 4. Code Examples

### Runnable Example

```python
import os
from data_processor import DataProcessor

def main():
    # 1. Ingestion
    dp = DataProcessor([10, 20, 30])
    dp.add_data(40)
    dp.batch_add([50, 60, 70, 80, 90, 100])
    print("Initial Data Count:", len(dp.get_summary_statistics()))

    # 2. Compute Summary Statistics
    stats = dp.get_summary_statistics()
    print("\n--- Summary Statistics ---")
    for key, value in stats.items():
        print(f"{key}: {value}")

    # 3. Filtering
    filtered = dp.filter_data(min_val=25, max_val=75)
    print("\nFiltered (25 <= x <= 75):", filtered)

    # 4. Linear Transformation: y = 0.1 * x + 5
    transformed = dp.transform_data(factor=0.1, offset=5.0)
    print("\nTransformed Data (0.1 * x + 5):", transformed)

    # 5. CSV Export and Import
    csv_file = "dataset_output.csv"
    try:
        dp.export_to_csv(csv_file)
        print(f"\nSuccessfully exported dataset to '{csv_file}'")

        # Reload data into a new DataProcessor instance
        new_dp = DataProcessor()
        new_dp.load_from_csv(csv_file)
        print("Reloaded Mean:", new_dp.calculate_mean())
    finally:
        if os.path.exists(csv_file):
            os.remove(csv_file)

if __name__ == "__main__":
    main()
```

---

## 5. Error Handling & Edge Cases

`DataProcessor` includes explicit input verification to maintain statistical and execution integrity:

1. **Empty Datasets**:
   * Calling statistical routines (`calculate_mean()`, `calculate_median()`, `calculate_variance()`, `calculate_std_dev()`, or `get_summary_statistics()`) on an empty dataset raises a `ValueError` with a descriptive message.
2. **Invalid Data Types**:
   * Passing `None`, strings, or complex objects to `add_data()`, `batch_add()`, or `__init__()` raises a `TypeError`.
   * Non-numeric factors or offsets in `transform_data()` trigger `TypeError`.
3. **File I/O and CSV Parsing**:
   * Attempting to import non-existent CSV files raises `FileNotFoundError`.
   * Unparseable rows or non-numeric tokens inside CSV files raise `ValueError`.
4. **Sample Variance with $n < 2$**:
   * Computing sample variance or sample standard deviation on a single-element dataset ($n=1$) raises `ValueError` to prevent division by zero ($n - 1 = 0$).