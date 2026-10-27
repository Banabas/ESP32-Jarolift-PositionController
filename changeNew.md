# v1.22.6

## what's new

Fixes [issue #12](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/12).

### State closed only when fully closed

After a stop between 1% and 25%, the state topic reported `closed`, and Home Assistant showed the shutter as closed although it was partly open. `closed` is now only sent at 0% and `open` only at 100%. In between, `stopped` is sent, and Home Assistant shows the shutter as open with the current position.

### Position while moving

While a calibrated shutter is moving, the estimated position is now sent about once per second. Before, the position topic only changed at the start and at the end of the move. The state stays `opening` / `closing` until the shutter stops.

### Faster command start

Commands are sent at most every 500 ms. The first command after a pause waited up to 500 ms for the next send cycle, and is now sent right away.

## changelog

- [FIX] state `closed` only at 0%, `stopped` between 1% and 99%
- [FEATURE] estimated position about once per second while moving
- [FEATURE] first command after a pause is sent without waiting for the send cycle
