# v1.22.9

## what's new

### Remote commands ignored for the position

Commands from a remote or sensor were sometimes ignored by the position tracking, e.g. STOP while the shutter was opening. The position then kept running to the end position, although the shutter had stopped. The receiver started decoding before the telegram was complete and read the channel bits 9-16 as 1, so the channel did not match the configured shutters. Decoding now waits for the complete telegram.

### Log of remote signals

The log line of a received remote signal was cut off after 128 characters, so the channel was missing. It is now shorter, shows the channel as hex value and the name at the end: `remote signal | serial: 0x02c58900 | cmd: STOP | channel: 0x0002 | name: ...`

## changelog

- [FIX] remote / sensor commands were sometimes ignored by the position tracking (incomplete telegram decoded)
- [FIX] log line of remote signals shows the channel
