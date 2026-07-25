# v1.21.3

## what's new

### Accurate position tracking for remote-controlled moves

Commands sent by a physical Jarolift remote (or a sensor acting as one) previously reset the stored position to a flat 0% / 100%, and STOP was ignored entirely. A shutter driven by hand - e.g. stopped halfway, then moved the other way - ended up with a wrong estimated position. Remote-triggered UP/DOWN/STOP now feed the same time-based position tracker that is used for the controller's own commands. Channels without a calibrated travel time keep the previous behaviour.

### More detail in the log

Start and end of a tracked move are now logged with channel name and position.

## changelog

- [FEATURE] Remote-triggered UP/DOWN/STOP update the time-based position tracker
- [FEATURE] Log start and end of tracked moves with channel name and position
