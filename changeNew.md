# v1.22.2

## what's new

### "Check for update" no longer crashes

Pressing "check for update" caused a panic reset when the GitHub request failed. In the GitHub OTA library the field `tag_name` of a release had no initial value, so after a failed request `freeRelease()` freed a random pointer. The library is now part of the project (`lib/ESP_Git_OTA`) with this fix, and failed requests write their HTTP error to the log.

### Update check works on the ESP32-S2

The Arduino core places all TLS buffers of mbedTLS in internal RAM. The ESP32-S2 rarely has a free block of ~16 KB there, so the HTTPS request to GitHub failed with "connection refused". On boards with PSRAM, buffers of 4 KB and more now go to PSRAM.

## changelog

- [FIX] Panic reset on "check for update" when the GitHub request fails (uninitialised `tag_name` in ESP_Git_OTA)
- [FIX] ESP32-S2: GitHub update check failed with "connection refused" - mbedTLS uses PSRAM for large buffers
- [CHANGE] ESP_Git_OTA is now included in `lib/` instead of being downloaded at build time
