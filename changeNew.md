# v1.22.1

## what's new

Fixes the crashes reported in [issue #5](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/5).

### ESP32-S2 no longer crashes when the WebUI is open

The ESP32-S2 build used the board definition of a classic ESP32. With it the embedded PSRAM of the S2 was never initialised, and only about 15 KB of internal RAM were left once WiFi and the web server were running. As soon as the WebUI was opened, sending the page data failed with an out-of-memory `abort()` - the ESP restarted in a loop, which also made the setup impossible.

- The S2 build now uses the `lolin_s2_mini` board definition (ESP32-S2FN4R2 with 2 MB PSRAM). PSRAM is now active and the free internal RAM rose from ~15 KB to ~65 KB.
- The WebUI log buffer (40 KB) is now allocated at startup and placed in PSRAM if available.
- Timer fields are sent to the WebUI in small packets, one channel or group at a time, and only when enough memory is free.
- On the S2 the log is now also printed on the USB port ("USB CDC On Boot"), and firmware uploads no longer need the BOOT button.

> [!NOTE]
> After this update the ESP32-S2 shows up as a different COM port, because its USB ID changed.

> [!IMPORTANT]
> The default CC1101 pins (GDO2 = GPIO22, MOSI = GPIO23) do not exist on the ESP32-S2. Set pins that exist on your board in the GPIO settings.

## changelog

- [FIX] ESP32-S2: crash loop (out of memory) as soon as the WebUI is opened - PSRAM is now used (#5)
- [FIX] Timer: "Go to position" defaults to 50 % instead of 0 % for new timers
- [CHANGE] ESP32-S2 build uses the `lolin_s2_mini` board definition with USB CDC on boot
- [CHANGE] WebUI log buffer is allocated in PSRAM if available
- [CHANGE] Timer fields are sent to the WebUI one channel/group per packet
