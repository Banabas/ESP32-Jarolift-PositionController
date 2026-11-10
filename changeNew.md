# v1.23.0

## what's new

Many of the radio changes in this release come from Reinhard-M, who measured the controller and the original remote with an SDR ([#17](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/17), details in [Doc/RF-Analyse.md](https://github.com/Banabas/ESP32-Jarolift-PositionController/blob/main/Doc/RF-Analyse.md)).

### Telnet is now off by default

The Telnet server (port 23, no login) can be switched on and off in the settings, without a restart. **It is off by default, also after the update.** If you use Telnet, switch it on again under Settings -> Telnet.

### More reliable sending

- The carrier frequency is now 433.92 MHz, the same as the original remotes. Before it was 433.945 MHz.
- The telegrams are now sent by the RMT hardware with exact timing. On the ESP32-S2, WiFi interrupts made single pulses too long, and the motor read the serial number shifted by 1-2 bits.
- The transmit power setting (about +10 dBm) is now actually written to the CC1101. Before it stayed at about 0 dBm.
- UP and DOWN are sent 5 times instead of 2 times, so one disturbed repetition no longer loses the command. STOP is still sent 2 times, because a repeated STOP means "hold STOP" (shade position).
- Partial moves are more precise: the travel time now starts when the first telegram was sent, not after all repetitions.

### Receiving

Noise was sometimes decoded as a remote telegram (e.g. `serial: 0x0ffeffbf | cmd: 0xb`). A telegram is now only used if the pulse timing is plausible and the KeeLoq check passes. Discarded telegrams appear only in the debug log.

### WiFi

- In a mesh (e.g. FRITZ!Repeater) the controller connects to the strongest access point instead of the first one found. Every 30 minutes it checks whether another access point is at least 8 dB stronger and switches to it.
- While the WiFi is off (e.g. at night), the log is no longer filled with `WiFi-Disconnected` every 2-3 seconds ([#15](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/15)). The disconnect is logged once, further attempts about every 5 minutes, and after reconnecting a summary shows how long the controller was offline.

### WebUI and OTA

- OTA update via WebUI no longer fails with "Could Not Activate The Firmware" ([#18](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/18)).
- The position sliders update live while a shutter moves, also for commands from timers, MQTT/HA or a remote.
- The log is sent to the browser in small packets, so the WebUI no longer disconnects when the log is refreshed (especially on the ESP32-S2).

### Diagnostics

- New Telnet command `cc1101 r <reg> | w <reg> <val> | agc` to read and write CC1101 registers (not saved).
- Debug log: RX statistics per minute and details of discarded telegrams.

## changelog

- [FEATURE] Telnet server can be switched on and off (off by default)
- [FEATURE] WiFi: connect to and roam to the strongest access point in a mesh
- [FEATURE] Telnet command `cc1101` to read/write CC1101 registers
- [FIX] carrier frequency 433.92 MHz instead of 433.945 MHz
- [FIX] telegrams sent by RMT hardware (shifted serial number on ESP32-S2)
- [FIX] transmit power was not written to the CC1101
- [FIX] UP/DOWN sent 5 times, STOP 2 times
- [FIX] travel time of partial moves starts with the first telegram
- [FIX] noise decoded as remote telegram
- [FIX] OTA update failed with "Could Not Activate The Firmware" (#18)
- [FIX] position sliders in the WebUI update live
- [FIX] WebUI disconnected when the log was refreshed
- [FIX] log flooded with "WiFi-Disconnected" while the WiFi is off (#15)
