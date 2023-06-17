# HID Protocol and communication

## Output (received from pc)

| Id | Function | format | description |
| --- | --- | --- | --- |
|0|...|...|...|
|1|Loadcell set filter|float.float|fc.q|
|2|Loadcell start stat|||
|3|Loadcell calibration|||

## Input (response to pc)

| Id | Function | format | description |
| --- | --- | --- | --- |
|0|Info|uint32_t.int32_t.uint16_t.uint32_t|Position.Speed.Current.Load|
|1|Loadcell get filter|float.float|fc.q|
|2|Loadcell get stat|float*5|Min.Max.Avg.StdD.StdE|
|3|Loadcell get calibration|float|Offset|