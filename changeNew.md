# v1.21.5

## what's new

Bug fix reported in [issue #8](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/8) - thanks to @Reinhard-M.

### Position 0-4 % via MQTT works

Shutter topics (`../cmd/shutter/N`) accepted the digits `0`-`4` as legacy commands (`0` = UP, `1` = DOWN, `2` = STOP, `3` = SHADE, `4` = SETSHADE). The same topic also takes a numeric target position, so sending position `0` opened the shutter instead of closing it, and positions 1-4 % never reached the position control. This also affected the Home Assistant position slider.

Shutter topics now only accept the text commands `UP`/`OPEN`, `DOWN`/`CLOSE`, `STOP`, `SHADE` and `SETSHADE`; every number from 0 to 100 is a target position.

> [!IMPORTANT]
> If you send the digits `0`-`4` as commands to `../cmd/shutter/N`, switch to the text commands. Group topics (`../cmd/group/N`) are unchanged and still accept the digits.

## changelog

- [FIX] Numeric payloads on shutter topics are always treated as target positions - `0` closes the shutter instead of opening it (#8)
- [BREAKING] Digit commands `0`-`4` are no longer accepted on `../cmd/shutter/N` - use `UP`, `DOWN`, `STOP`, `SHADE`, `SETSHADE`
