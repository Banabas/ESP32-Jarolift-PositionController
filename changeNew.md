# v1.22.10

## what's new

Fixes [issue #15](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/15).

### No WiFi reconnect

After the WiFi connection was lost (e.g. router restart or WiFi switched off at night), the controller did not connect again until it was restarted. The disconnect was not registered, so the controller still assumed it was connected and never tried to reconnect. The disconnect is now detected, and the controller tries to reconnect every 30 seconds until the WiFi is back. It no longer restarts after 5 failed attempts, so a WiFi switched off at night does not cause a restart every few minutes.

## changelog

- [FIX] WiFi reconnects after the connection was lost
