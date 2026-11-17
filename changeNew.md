# v1.24.1

## what's new

The fix in this release comes from Reinhard-M ([#22](https://github.com/Banabas/ESP32-Jarolift-PositionController/pull/22), details in [Doc/RF-Analyse.md](https://github.com/Banabas/ESP32-Jarolift-PositionController/blob/main/Doc/RF-Analyse.md), finding 10).

### CC1101 no longer "hangs"

- The CS line of the CC1101 was never switched, the chip was selected all the time. Between the register accesses the SPI lines SCK/MOSI were detached and floating. A single noise edge could shift the SPI framing of the CC1101 for good: the controller did not send any more (or the receiver showed a permanently high edge count), and only switching the power off and on helped - a restart of the ESP did not.
- CS is now switched for every access, and the SPI lines stay attached.
- If the CC1101 does not enter the transmit state, it is re-initialized and the telegram is sent again.
- Once per minute the controller checks that the CC1101 is in receive mode. If not, it is re-initialized.
- Each re-initialization is logged as a warning `CC1101 re-init: ...`.

## changelog

- [FIX] CC1101 could hang until a power cycle, CS line was never switched (#22)
- [FEATURE] automatic re-initialization of the CC1101 if it does not enter TX or leaves RX (#22)
