# v1.22.8

## what's new

Fixes [issue #13](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/13).

### Update from GitHub picked the wrong image

The update from GitHub ("check for update") searched for the first file containing the chip series. On an ESP32 this also matched the ESP32-S2 image, and the update failed with `mismatch chip ID` and `Firmware update failed: 5`. On an ESP32-S3 with 4 MB flash, the image for 16 MB could be picked. The firmware now searches for the image of exactly its own build variant.

> [!IMPORTANT]
> On ESP32 and ESP32-S3 (4 MB), the update **to** v1.22.8 can still fail via GitHub, because the old version picks the file. Please install v1.22.8 once via WebUI -> Tools -> OTA Update with the matching `_ota.bin` file. From then on, the update from GitHub works again.

## changelog

- [FIX] update from GitHub picks the image of the own build variant (ESP32 got the ESP32-S2 image)
