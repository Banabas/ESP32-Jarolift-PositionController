ESP32-Jarolift-PositionController v1.21.4
=========================================

Unterstuetzte Chips
-------------------
  esp32          - ESP32-WROOM, ESP32-WROVER, ESP32-MINI
  esp32s2        - ESP32-S2
  esp32s3        - ESP32-S3 (4 MB flash)
  esp32s3_16mb   - ESP32-S3 (16 MB flash)

Dateien in diesem Release
--------------------------

  ESP32-Jarolift-PositionController_v1.21.4_esp32_flash.bin
    -> Erstmaliges Flashen fuer ESP32-WROOM, ESP32-WROVER, ESP32-MINI
    -> esptool.py --chip esp32 --baud 460800 write_flash 0x0 ESP32-Jarolift-PositionController_v1.21.4_esp32_flash.bin

  ESP32-Jarolift-PositionController_v1.21.4_esp32_ota.bin
    -> OTA-Update fuer ESP32-WROOM, ESP32-WROVER, ESP32-MINI via WebUI -> Tools -> OTA Update

  ESP32-Jarolift-PositionController_v1.21.4_esp32s2_flash.bin
    -> Erstmaliges Flashen fuer ESP32-S2
    -> esptool.py --chip esp32s2 --baud 460800 write_flash 0x0 ESP32-Jarolift-PositionController_v1.21.4_esp32s2_flash.bin

  ESP32-Jarolift-PositionController_v1.21.4_esp32s2_ota.bin
    -> OTA-Update fuer ESP32-S2 via WebUI -> Tools -> OTA Update

  ESP32-Jarolift-PositionController_v1.21.4_esp32s3_flash.bin
    -> Erstmaliges Flashen fuer ESP32-S3 (4 MB flash)
    -> esptool.py --chip esp32s3 --baud 460800 write_flash 0x0 ESP32-Jarolift-PositionController_v1.21.4_esp32s3_flash.bin

  ESP32-Jarolift-PositionController_v1.21.4_esp32s3_ota.bin
    -> OTA-Update fuer ESP32-S3 (4 MB flash) via WebUI -> Tools -> OTA Update

  ESP32-Jarolift-PositionController_v1.21.4_esp32s3_16mb_flash.bin
    -> Erstmaliges Flashen fuer ESP32-S3 (16 MB flash)
    -> esptool.py --chip esp32s3 --baud 460800 write_flash 0x0 ESP32-Jarolift-PositionController_v1.21.4_esp32s3_16mb_flash.bin

  ESP32-Jarolift-PositionController_v1.21.4_esp32s3_16mb_ota.bin
    -> OTA-Update fuer ESP32-S3 (16 MB flash) via WebUI -> Tools -> OTA Update

Positionssteuerung
------------------
Dieser Build enthaelt zeitbasierte Positionssteuerung (0-100%) fuer Jarolift TDEF Rolllaeden.
0% = geschlossen, 100% = offen (entspricht der Home Assistant Konvention).
Kalibrierung erforderlich pro Kanal (Service-Seite -> Laufzeit-Kalibrierung).

Quellcode
---------
https://github.com/Banabas/ESP32-Jarolift-PositionController
