# Funkanalyse und Verbesserungen der Sendestrecke (November 2026)

Diese Notiz dokumentiert eine Untersuchung mit einem SDR-Empfänger an zwei Controllern mit dieser Firmware und die daraus entstandenen Änderungen.

## Ausgangslage

- Zwei Controller mit nahezu identischer Hardware (CC1101-Modul, Drahtantenne bzw. Stabantenne):
  - **ESP32** (Dual-Core, „ESP32-mini“): funktionierte meistens.
  - **ESP32-S2** (Single-Core, Lolin S2 mini): setzte kaum einen Befehl erfolgreich ab; bei aktivem MQTT gab es zusätzlich gelegentliche Resets.
- Die Motoren reagierten teils gar nicht, teils erst nach mehrfachem Senden.

## Messaufbau

- SDR: NooElec NESDR SMArTee XTR (RTL2832U + E4000, TCXO 0,5 ppm) mit SDR++
- Samplerate 2,048 MSPS, Gain fest 19 dB, AGC aus, FFT-Größe 65536, FFT Hold
- Abstand Sender – SDR 2,5 m bzw. 12,5 m, nahezu Sichtverbindung
- Frequenzwerte sind aus den Screenshots abgelesen (Genauigkeit etwa ±2 kHz)

## Befund 1: Trägerfrequenz 25 kHz zu hoch

Die Firmware programmierte `FREQ = 0x10B0B0`:
`26 MHz × 0x10B0B0 / 2^16 = 433,9448 MHz`

Beide Controller sendeten gemessen auf etwa 433,943 MHz, die Quarze waren also genau; die Abweichung kam allein aus dem Registerwert (aus dem Vorprojekt übernommen).

| vorher ESP32-S2 | vorher ESP32 |
|---|---|
| ![vorher ESP32-S2](rf-analyse/1_vorher_esp32s2_433945.png) | ![vorher ESP32](rf-analyse/2_vorher_esp32_433945.png) |

Mit `FREQ0 = 0x71` (`0x10B071` = 433,9198 MHz) liegen beide Sender auf 433,92 MHz (rote Linie = 433,920 MHz). Diese Korrektur ist bereits in `main` (Commit 7f0f4be).

| nachher ESP32 | nachher ESP32-S2 |
|---|---|
| ![nachher ESP32](rf-analyse/3_nachher_esp32_43392.png) | ![nachher ESP32-S2](rf-analyse/4_nachher_esp32s2_43392.png) |

Zum Vergleich eine Original-Jarolift-Fernbedienung: sie sendet auf etwa **433,814 MHz** (rund 106 kHz unter 433,92 MHz) und wird trotzdem verstanden. Die Empfänger in den Motoren sind also sehr breitbandig – die Frequenz war **nicht** die Hauptursache der Ausfälle, 433,92 MHz ist aber der saubere Sollwert.

![Original-Fernbedienung](rf-analyse/5_original_fernbedienung.png)

(Der kleinere Puls bei etwa 433,944 MHz im Bild „nachher ESP32“ stammt von einem fremden Sender; er war auch bei abgeschalteten Controllern zu sehen.)

## Befund 2: Verschobene Bits beim ESP32-S2 → Senden per RMT

Der zweite Controller (ESP32) empfängt die Telegramme des S2 mit. Im Log tauchte dieselbe Seriennummer in vielen „verschobenen“ Varianten auf:

| empfangen | Verhältnis zu 0x01305006 |
|---|---|
| 0x00982803 | 1 Bit nach rechts |
| 0x004C1401 | 2 Bit nach rechts |
| 0x0260A00C | 1 Bit nach links |
| 0x00982802, 0x0260A00D, 0x02609006 | 1 Bit verschoben + Bitfehler |

Ursache: Die Pulse wurden mit `digitalWrite()` / `delayMicroseconds()` erzeugt (ca. 100 ms pro Telegramm). Auf dem **Single-Core-ESP32-S2** unterbrechen WLAN-Interrupts und Systemtasks diese Schleife, einzelne Pulse werden um einige hundert µs zu lang und der Empfänger verzählt sich. Auf dem Dual-Core-ESP32 laufen WLAN und Programm auf getrennten Kernen, deshalb trat das dort kaum auf.

**Änderung:** Das komplette Telegramm wird als RMT-Symbole (86 Symbole, 1 µs Auflösung) aufgebaut und von der RMT-Hardware ausgegeben. Pulslängen, Präambel, Sync-Pause und die 16 ms zwischen Wiederholungen sind unverändert, aber jetzt mikrosekundengenau. Während der Ausgabe wartet der Task blockierend auf das Ende (Semaphore), WLAN und MQTT laufen weiter. Das sollte auch die MQTT-Resets auf dem S2 beheben (in den bisherigen Tests traten keine mehr auf).

## Befund 3: Sendeleistung wurde nie gesetzt

```cpp
cc1101_.init();                          // schreibt PATABLE[0] = paTableByte (0x60)
...
cc1101_.setTxPowerAmp(PA_LongDistance);  // ändert nur die Variable -> keine Wirkung
```

Gesendet wurde mit 0x60 (etwa 0 dBm) statt 0xC0 (etwa +10 dBm).

**Änderung:** `setTxPowerAmp(PA_LongDistance)` wird **vor** `init()` aufgerufen; `init()` schreibt den Wert wie bisher per Einzelzugriff.

**Wichtiger Hinweis für spätere Änderungen:** Die CS-Leitung des CC1101 wird nie auf HIGH gesetzt (`SPI.begin()` treibt CS nicht, der Pin bleibt nach `pinMode(OUTPUT)` dauerhaft LOW). Ein **Burst-Zugriff** (`writeBurstReg`) wird beim CC1101 aber erst mit CS = HIGH beendet. Ein erster Versuch, PATABLE[0..1] per Burst zu schreiben, hat deshalb alle folgenden SPI-Kommandos (STX, Statusabfragen) als PATABLE-Daten verschluckt – der Sender war stumm, MARCSTATE las 0x1F. Burst-Zugriffe also nur mit echter CS-Steuerung verwenden.

Zur Zuordnung der PATABLE im asynchronen OOK-Modus (FREND0 = 0x11): GDO0 LOW = Träger an = PATABLE[0], GDO0 HIGH = Träger aus = PATABLE[1].

## Befund 4: Wiederholungen

UP/DOWN wurden nur 2-mal gesendet (ca. 0,23 s). Eine gestörte Wiederholung reichte, um den Befehl zu verlieren.

**Änderung:** UP/DOWN werden 5-mal gesendet (alle mit demselben Zählerstand, der Motor führt den Befehl nur einmal aus). **STOP bleibt bei 2**, weil ein wiederholtes/gehaltenes STOP vom Motor als eigene Funktion (Zwischenposition/Shade) gewertet wird. SHADE (20-mal), Anlern- und Endpunkt-Sequenzen sind unverändert.

## Befund 5: WLAN im Mesh (FRITZ!Repeater)

Der S2 hing trotz eines Repeaters in 2 m Entfernung mit −72 dBm an einem entfernten Access Point, mit gelegentlichen Abbrüchen. `WiFi.begin()` verbindet sich standardmäßig mit dem **ersten** gefundenen AP, und der ESP32 unterstützt kein Mesh-Steering (802.11k/v).

**Änderungen:**
- Vor jedem Verbinden: Scan aller Kanäle und Auswahl des stärksten AP (`WIFI_ALL_CHANNEL_SCAN`, `WIFI_CONNECT_AP_BY_SIGNAL`). Ergebnis: −49 dBm.
- Periodisches Roaming (ähnlich Tasmota SetOption57): alle 30 Minuten asynchroner Scan nach der eigenen SSID; ist ein anderer AP mindestens 8 dB stärker, wird gezielt auf dessen BSSID umgeschaltet.
- Log nach dem Verbinden: `WiFi AP: BSSID … | channel … | RSSI … dBm`

## Diagnose im Log

Nach jedem Senden:

```
I JARO-LIB: TX via RMT: 86 symbols, 5/5 repetitions sent, MARCSTATE 0x13
```

- `86 symbols, n/n`: RMT hat alle Wiederholungen ausgegeben
- `MARCSTATE 0x13` (0x13–0x15): CC1101 war im Sendezustand

Falls der GDO0-Pin von einem anderen Treiber übernommen wurde, wird RMT automatisch neu initialisiert (Warnung im Log).

## Ergebnis

Mit allen Änderungen führt der Motor am ESP32-S2 jeden Befehl aus, auch kurz hintereinander gesendete; in den bisherigen Tests traten keine Resets mehr auf. Beide Controller senden auf 433,92 MHz.

## Hinweis für Anwender

Zwei Controller dürfen **nicht dieselbe Seriennummer** verwenden. Jeder Controller führt seinen eigenen Zähler; der Motor verwirft Telegramme mit einem Zählerstand, der hinter dem zuletzt akzeptierten liegt. Bei gleicher Seriennummer funktioniert dann meist nur einer der beiden zuverlässig.

## Commits

1. TX: Telegramme per RMT statt Software-Timing senden
2. CC1101: Sendeleistung – zunächst per Burst, siehe Befund 3, korrigiert in Schritt 6
3. TX: UP/DOWN/STOP 5-mal statt 2-mal senden
4. TX: STOP wieder nur 2-mal senden
5. TX: RMT-Diagnose und automatische Neuinitialisierung
6. CC1101: PATABLE nicht per Burst schreiben (setTxPowerAmp vor init)
7. WiFi: im Mesh mit dem stärksten Access Point verbinden
8. WiFi: periodisches Roaming zum stärksten Access Point
