# v1.22.4

## what's new

Fixes [issue #10](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/10).

### Configurable shade position

After a SHADE command from the WebUI or Home Assistant, the shutter position was not updated and stayed at 100%. Home Assistant then saw no reason to send OPEN anymore. Groups and remotes reported a fixed 10% but did not update the internal position tracking.

Each channel now has a **shade position (%)** in the settings (default 10%). After SHADE (single channel, group or remote) this position is taken over by the position tracking and reported via MQTT. "Set shade" in the service menu automatically stores the current position as shade position when the channel is calibrated.

### Unique MQTT client ID

The MQTT client ID was the hostname. Several controllers with the same hostname (e.g. the default `ESP32-Jarolift`) kicked each other off the broker, which showed up as "TCP DISCONNECTED" and a `status` of `offline`. The client ID now gets the last 3 bytes of the MAC address appended (e.g. `ESP32-Jarolift-A1B2C3`), so this can no longer happen.

### Shutter state opening / closing

Suggested in [discussion #11](https://github.com/Banabas/ESP32-Jarolift-PositionController/discussions/11). Calibrated shutters now report `opening` or `closing` while moving, on the new topic `status/shutter/N/state`. The position topic keeps the last position during the move, and the new position is sent once the target or end position is reached. Before, the end position (0% or 100%) was sent right at the start, even when the shutter was only driven to 50%. Home Assistant uses the new state topic automatically.

### Group commands update the position

Group UP / DOWN / STOP did not update the position tracking of the shutters in the group. They now behave like the single shutter commands.

### Position command in the opposite direction

When a moving shutter got a target position in the opposite direction, the STOP sent in between discarded the target and the shutter ran to the end position. The target is now kept.

## changelog

- [FIX] SHADE updates the shutter position, so Home Assistant can open the shutter again
- [FEATURE] configurable shade position per channel
- [FIX] unique MQTT client ID, several controllers no longer kick each other off the broker
- [FEATURE] shutter state opening / closing while moving (status/shutter/N/state)
- [FIX] group UP / DOWN / STOP update the position tracking
- [FIX] position command in the opposite direction no longer runs to the end position
