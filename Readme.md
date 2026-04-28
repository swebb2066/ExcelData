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
  - &NamePrefix xxxxx
  - &Location xxxxx
Input:
  Path: [*Location, "*/", *NamePrefix, " Personal Expenses.xls"]
  Sheet: Transactions
---
````

Example 2: Extract totals
----------
```yaml
Parameters:
  - &NamePrefix xxxxxx
Input:
  Path: [*NamePrefix, "*Personal Expenses.xls"]
  Sheet: Transactions
Accumulate:
  - Name: TotalPlannedArea
    Value: !area [PartTrackArea]
  - Name: TotalBadAngleArea
    Value: !area [ExtremeAngleToSurfacePortion, PartTrackArea, x]
  - Name: TotalTargetedArea
    Value: !area [SectionTrackArea]
  - Name: Duration
    Value: !duration [TotalDuration]
Output:
  - Name: Task
    Value: *NamePrefix
    IsKey: true
  - Name: Total Targeted Area (m^2)
    Value: !area [TotalTargetedArea]
  - Name: Total Planned Area (m^2)
    Value: !area [TotalPlannedArea]
  - Name: Planned %
    Value: !area [TotalTargetedArea, TotalPlannedArea, /, 100, x]
  - Name: Bad Angle Area (m^2)
    Value: !area [TotalBadAngleArea]
  - Name: Effective %
    Value: !area [TotalTargetedArea, TotalBadAngleArea, TotalPlannedArea, -, /, 100, x]
  - Name: Total Duration (hrs)
    Value: !duration [3600, 2, Duration, /, /]
---
```

Example 3: Extract totals from selected nodes
----------
```yaml
ParameterCombinations:
- Name: Location
  Value: [ Front, Back ]
- Name: Dataset
  Value: [ Wagon_1, Wagon_2, Wagon_3, Wagon_4, Wagon_5
         , Wagon_6, Wagon_7, Wagon_8, Wagon_9, Wagon_10]
Parameters:
  - &Location xxxxxx
  - &Dataset dddddd
InputPath: [*Location, /, *Location, " ", *Dataset, " */*Personal Expenses.xls"]
Select:
  SectionName: [., Section, ".*Torsion Box"]
Input:
  ExtremeAngleToSurfacePortion: [Parts,.,ExtremeAngleToSurfacePortion]
  PartTrackArea: [Parts,.,TrackArea]
  SectionDuration: [Duration]
  SectionTrackArea: [TrackArea]
Accumulate:
  - Name: TotalPlannedArea
    Value: !area [PartTrackArea]
  - Name: TotalBadAngleArea
    Value: !area [ExtremeAngleToSurfacePortion, PartTrackArea, x]
  - Name: TotalTargetedArea
    Value: !area [SectionTrackArea]
  - Name: TotalDuration
    Value: !duration [SectionDuration]
Output:
  - Name: Torsion Box
    Value: [ *Location, " ", *Dataset ]
    IsKey: true
  - Name: Total Targeted Area (m^2)
    Value: !area [TotalTargetedArea]
  - Name: Total Planned Area (m^2)
    Value: !area [TotalPlannedArea]
  - Name: Planned %
    Value: !area [TotalTargetedArea, TotalPlannedArea, /, 100, x]
  - Name: Bad Angle Area (m^2)
    Value: !area [TotalBadAngleArea]
  - Name: Effective %
    Value: !area [TotalTargetedArea, TotalBadAngleArea, TotalPlannedArea, -, /, 100, x]
  - Name: Total Duration (hrs)
    Value: !duration [3600, TotalDuration, /]
```

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

