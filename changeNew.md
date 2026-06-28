# v1.21.0

## what's new

This release fixes several bugs found during review of the position-control code introduced in v1.20.0, and adds two smaller usability improvements.

> [!WARNING]
> **Breaking change:** the position percentage convention has been inverted to match Home Assistant's native cover convention:
> **0% = fully closed, 100% = fully open** (previously it was the other way round).
> If you have MQTT automations, scripts, or Node-RED flows that send/read a numeric shutter position, update them to the new convention. Home Assistant's auto-discovered cover entities require no changes - they already worked the "right way round" before via an internal inversion that has now been removed.

### Manual travel-time correction

The automatically measured travel times (from calibration) can now be fine-tuned by hand on the **Settings** page, per channel, in seconds (fields "Travel time DOWN" / "Travel time UP"). Useful if the calibrated value drifts slightly from the shutter's real travel time.

### Channel names in the log

Log messages for shutter and position commands now show the configured channel name (e.g. "Living Room") instead of just the channel number, making the log easier to read with many channels in use.

## changelog

- [BREAKING] Position convention inverted: 0% = closed, 100% = open (matches Home Assistant; MQTT discovery position template simplified accordingly)
- [FEATURE] Manual per-channel travel-time correction (seconds) on the Settings page
- [FEATURE] Log output shows configured channel names instead of channel numbers
- [FIX] Format-string vulnerability: several WebUI config fields passed user-supplied text directly as a printf format string instead of as `%s` data
- [FIX] Position control: retargeting to a new position while already moving in the same direction no longer loses its stop timer (shutter no longer overruns to the end-stop)
- [FIX] Position control: stop-time calculation no longer wraps to a huge value when the time-based position estimate drifts past the target, which could send the shutter to the wrong end-stop
- [FIX] Config: MQTT password could be left as raw ciphertext if decryption failed after a corrupted/partial config read, causing a permanent reconnect/reboot loop
- [FIX] Timer: weekend override no longer fires the other direction's normal weekday schedule when only one direction (UP or DOWN) has weekend mode enabled
- [FIX] Config: GPIO duplicate-pin check had an off-by-one and never checked the 20th configured pin
- [FIX] Timer: day-rollover guard used a 32-bit `int` instead of `time_t`, which could permanently stop all timers from firing after a long uptime
- [FIX] Position control: calibration/log timing used an extra `millis()` call that could make the logged "stop in N ms" value wrap to a huge, misleading number
