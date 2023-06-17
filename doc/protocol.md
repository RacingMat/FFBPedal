# HID Protocol and communication

## Output (received from pc)

| Id | Function | format | description |
| --- | --- | --- | --- |
|0|...|...|...|
|1|TODO Loadcell set filter|float.float|fc.q|
|2|TODO Loadcell start stat|||
|3|TODO Loadcell calibration|||

## Input (response to pc)

| Id | Function | format | description |
| --- | --- | --- | --- |
|0|Info|uint32_t.int32_t.int16_t.uint32_t|Position.Speed.MotorCommand.Load|
|1|TODO Loadcell get filter|float.float|fc.q|
|2|TODO Loadcell get stat|float*5|Min.Max.Avg.StdD.StdE|
|3|TODO Loadcell get calibration|float|Offset|