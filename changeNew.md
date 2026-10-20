# v1.22.3

## what's new

Fixes [issue #6](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/6).

### Remote control only moves the position of the selected shutter

When a remote was registered for several shutters (e.g. a TDR16 for 7 shutters), every button press updated the position of all shutters in its mask, no matter which channel was selected on the remote. Now only the shutters whose channel was actually sent are updated - remote channel n belongs to shutter n. Single-channel remotes that are assigned to other shutters keep updating their whole mask.

## changelog

- [FIX] Remote commands only update the position of the shutters on the selected remote channel(s)
