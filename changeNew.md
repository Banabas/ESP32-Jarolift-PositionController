# v1.24.0

## what's new

The receiver changes in this release come from Reinhard-M ([#20](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/20), [#21](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/21), details in [Doc/RF-Analyse.md](https://github.com/Banabas/ESP32-Jarolift-PositionController/blob/main/Doc/RF-Analyse.md)).

### Better reception of the original remotes

- The CC1101 now uses the receive settings of SIGNALduinoAdv (Ralf9): 325 kHz bandwidth, about 5.6 kBaud, adjusted AGC and OOK threshold. Before, the original remote TDRC16 was often not received at all; now it is received reliably, also over longer distances.
- Short pulses down to 200 µs are accepted (the TDRC16 sends 290-420 µs).
- A short noise spike inside a pulse no longer splits the pulse in two. Before, the telegram was discarded or the serial number was read shifted.
- The noise in idle state drops a lot (on the ESP32-S2 from several 100,000 to a few 1,000 edges per minute), which also lowers the interrupt load.

### RX statistics

- The receiver is measured every minute. The debug log shows an hourly summary, the Telnet command `cc1101 stat` the last minute.
- If there are more than 50,000 edges or at least 5 discarded telegrams per minute (e.g. a disturbing device nearby), the info log shows `RX disturbed` once, and `RX ok again` when the disturbance has ended.

### WiFi

- The controller now tries to reconnect every 30 seconds on its own, independent of the auto-reconnect of the ESP32 core. The core does not reconnect after some disconnect reasons (e.g. `reason 209`, seen with WPA2/WPA3 mixed mode), and the controller stayed offline until a restart ([#15](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/15)).

### Backup

- The device counter is now saved in the configuration backup and restored with it. Before, after restoring a backup on a new ESP (or after erasing the flash) the counter started at 1 again, the motors ignored all telegrams and had to be learned again. A restored backup never sets the counter back.
- The "Export" and "Show" buttons in the WebUI no longer send the upload form at the same time.

## changelog

- [FEATURE] receive settings of SIGNALduinoAdv, original remotes are received reliably (#20)
- [FEATURE] RX statistics, `RX disturbed` / `RX ok again` in the log
- [FEATURE] device counter is part of the configuration backup
- [FIX] noise spikes inside a pulse split the pulse (#20, #21)
- [FIX] WiFi stayed offline after some disconnect reasons, e.g. 209 (#15)
- [FIX] Export/Show buttons in the WebUI sent the upload form
