# v1.22.0

## what's new

Feature request from [discussion #2](https://github.com/Banabas/ESP32-Jarolift-PositionController/discussions/2) - thanks to @ruppik.

### Timer can move shutters to a position

Every channel and group timer now has a third event **Go to position** next to UP and DOWN. It moves the shutter to a configurable target position (0 % = closed, 100 % = open), for example a shading position at noon. This works for motors that do not support the built-in shade function.

The event offers the same trigger options as UP and DOWN: fixed time, sunrise/sunset with astro mode and offset, and the weekend override.

> [!NOTE]
> Position moves need a calibrated travel time. Uncalibrated channels ignore the event and write a warning to the log. For group timers each channel in the group is moved separately.

## changelog

- [FEATURE] Timer: new "Go to position" event per channel and group timer with a target position in % (#2)
