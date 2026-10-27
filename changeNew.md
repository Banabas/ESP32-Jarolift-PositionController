# v1.22.7

## what's new

### Calibration result shows the stored travel times

At the end of the calibration, the WebUI showed the time of the browser stopwatch, measured from the button click. The ESP measures from the moment the RF command is sent and stores that time, so the result shown could be up to 2 s longer than the stored value (reported in [issue #12](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/12)). The result now shows the travel times stored on the ESP. The running stopwatch during the calibration is only a guide.

## changelog

- [FIX] calibration result shows the stored travel times instead of the browser stopwatch
