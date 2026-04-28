A command line utility to extract data from Excel files using Excel's COM interface.

Allowed options:

| Flag | Description |
| ---- | ----------- |
|  -h [ --help ]         |  display optional parameter descriptions |
| | |
|  -r [ --input-root ] arg | The directory to search for input data. |
| | |
|  -m [ --map-file ] arg |  The name (default excel2csv.yaml) of a (yaml format) file |
| | mapping the input yaml to the output comma separated values. |
| | See [Map file syntax] for details. |
| | |
|  -p [ --map-params ] arg | The name of a file containing a (yaml format) list of parameter values. |
| | See [Parameter file syntax] for details. |

Map file syntax
===============

Each element in the input list has a variable name and path to the value.
Each element in the output list specifies a name and a value (optionally as a reverse polish calculation).

Example 1: Extract all transaction sheets
----------
```yaml
Parameters:
  - Name: NamePrefix
    Value: [19, 20]
  - Name: Location
    Value: [Family Trust, Super Fund]
Input:
  Path: [*Location, "*/", *NamePrefix, " Personal Expenses.xls"]
  Sheet: Transactions
---
````

Example 2: Extract a cell range from transaction sheets
----------
```yaml
Parameters:
  - Name: NamePrefix
    Value: [2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023]
  - Name: Location
    Value: [Family Trust, Super Fund]
Input:
  Path: [*Location, "*/", *NamePrefix, " Personal Expenses.xls"]
  Sheet: Transactions
  Cells: A1:D999
---
````
Parameter file syntax
=====================

Each list element specifies a variable name and list of value.
Output is generated for each combination in the cross product of variable values.

Example 1:
----------
```yaml
- Name: NamePrefix
  Value: [19, 20]
- Name: Location
  Value: [Family Trust, Super Fund]
---
```

Example 2:
----------
```yaml
- Name: NamePrefix
  Value: [ 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023 ]
---
```

The list of parameter values may alternatively be provided
in the 'ParameterCombinations' section of the map file.

