# v1.22.4

## what's new

Fixes [issue #10](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/10).

### Configurable shade position

After a SHADE command from the WebUI or Home Assistant, the shutter position was not updated and stayed at 100%. Home Assistant then saw no reason to send OPEN anymore. Groups and remotes reported a fixed 10% but did not update the internal position tracking.

Each channel now has a **shade position (%)** in the settings (default 10%). After SHADE (single channel, group or remote) this position is taken over by the position tracking and reported via MQTT. "Set shade" in the service menu automatically stores the current position as shade position when the channel is calibrated.

## changelog

- [FIX] SHADE updates the shutter position, so Home Assistant can open the shutter again
- [FEATURE] configurable shade position per channel
