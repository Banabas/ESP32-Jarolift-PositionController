# v1.21.4

## what's new

Bug fixes reported in [issue #1](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/1) - thanks to @airwolf-it for the detailed report. Three of them are inherited from upstream.

### Reception now works right after boot

`begin()` attached the RX interrupt but never put the CC1101 into RX state, leaving the chip in IDLE. Reception only started working once the controller had transmitted something, which enters RX as a side effect - so on a freshly booted device remote signals were silently ignored.

### The "new learn mode" switch works

Two problems kept the switch from doing anything. A copy/paste leftover from the log-level handling forced `learn_mode` back to on whenever it was stored as off, and `setLegacyLearnMode()` was never called, so the configured value never reached the controller. Both are fixed; the switch now also takes effect immediately instead of only after a restart. Receivers that need the legacy rear-button learn method (button code `0x1`) can be used again.

### Builds on Linux

`Dusk2Dawn` included `<Math.h>` instead of `<math.h>`, which fails on case-sensitive filesystems.

## changelog

- [FIX] CC1101 now enters RX state at the end of `begin()` - remote reception works on a freshly booted device
- [FIX] `learn_mode` is no longer forced back to on when saved or loaded as off
- [FIX] `setLegacyLearnMode()` is called from `jaroliftSetup()`, so the WebUI switch reaches the controller
- [FIX] `Dusk2Dawn` includes `<math.h>` in the correct case - fixes the build on Linux
