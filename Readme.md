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
|  -p [ --map-params ] arg | The name of a file containing a (yaml format) list of parameter values from which combinations are constructed. |
| | See [Parameter file syntax] for details. |

Map file syntax
===============

Each element in the input section nominates the file path pattern and the data to extract.

Example 1: Extract the Transactions sheet from all selected workbooks
----------
```yaml
Input:
  Path: "2[0-9]06 Personal Expenses.xls"
  Sheet: Transactions
---
````

Example 2: Extract a named cell range from all selected workbooks
----------
```yaml
Input:
  Path: "2[0-9]06 Personal Expenses.xls"
  Name: Sum_of_Amounts
---
````

Example 3: Extract the Transactions sheet from all selected workbooks using all combinations of Location and NamePrefix
----------
```yaml
ParameterCombinations:
  - Name: NamePrefix
    Value: [19, 20]
  - Name: Location
    Value: [Family Trust, Super Fund]
Parameters:\n"
  - &NamePrefix xxxxx
  - &Location xxxxx
Input:
  Path: [*Location, "*/", *NamePrefix, " Personal Expenses.xls"]
  Sheet: Transactions
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

