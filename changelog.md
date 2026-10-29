**Changelog**

# v1.22.9

## what's new

### Remote commands ignored for the position

Commands from a remote or sensor were sometimes ignored by the position tracking, e.g. STOP while the shutter was opening. The position then kept running to the end position, although the shutter had stopped. The receiver started decoding before the telegram was complete and read the channel bits 9-16 as 1, so the channel did not match the configured shutters. Decoding now waits for the complete telegram.

### Log of remote signals

The log line of a received remote signal was cut off after 128 characters, so the channel was missing. It is now shorter, shows the channel as hex value and the name at the end: `remote signal | serial: 0x02c58900 | cmd: STOP | channel: 0x0002 | name: ...`

## changelog

- [FIX] remote / sensor commands were sometimes ignored by the position tracking (incomplete telegram decoded)
- [FIX] log line of remote signals shows the channel

---

# v1.22.8

## what's new

Fixes [issue #13](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/13).

### Update from GitHub picked the wrong image

The update from GitHub ("check for update") searched for the first file containing the chip series. On an ESP32 this also matched the ESP32-S2 image, and the update failed with `mismatch chip ID` and `Firmware update failed: 5`. On an ESP32-S3 with 4 MB flash, the image for 16 MB could be picked. The firmware now searches for the image of exactly its own build variant.

> [!IMPORTANT]
> On ESP32 and ESP32-S3 (4 MB), the update **to** v1.22.8 can still fail via GitHub, because the old version picks the file. Please install v1.22.8 once via WebUI -> Tools -> OTA Update with the matching `_ota.bin` file. From then on, the update from GitHub works again.

## changelog

- [FIX] update from GitHub picks the image of the own build variant (ESP32 got the ESP32-S2 image)

---

# v1.22.7

## what's new

### Calibration result shows the stored travel times

At the end of the calibration, the WebUI showed the time of the browser stopwatch, measured from the button click. The ESP measures from the moment the RF command is sent and stores that time, so the result shown could be up to 2 s longer than the stored value (reported in [issue #12](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/12)). The result now shows the travel times stored on the ESP. The running stopwatch during the calibration is only a guide.

## changelog

- [FIX] calibration result shows the stored travel times instead of the browser stopwatch

---

# v1.22.6

## what's new

Fixes [issue #12](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/12).

### State closed only when fully closed

After a stop between 1% and 25%, the state topic reported `closed`, and Home Assistant showed the shutter as closed although it was partly open. `closed` is now only sent at 0% and `open` only at 100%. In between, `stopped` is sent, and Home Assistant shows the shutter as open with the current position.

### Position while moving

While a calibrated shutter is moving, the estimated position is now sent about once per second. Before, the position topic only changed at the start and at the end of the move. The state stays `opening` / `closing` until the shutter stops.

### SHADE reports opening / closing

For calibrated shutters, SHADE now reports `opening` or `closing` right before the command is sent, depending on the current position and the shade position. During the move the estimated position is sent, and the shade position once it is reached. Before, the shade position was reported immediately. SHADE is sent as a long STOP press (about 0.5 s), so the motor starts slightly later than with UP / DOWN.

### Faster command start

Commands are sent at most every 500 ms. The first command after a pause waited up to 500 ms for the next send cycle, and is now sent right away.

## changelog

- [FIX] state `closed` only at 0%, `stopped` between 1% and 99%
- [FEATURE] estimated position about once per second while moving
- [FEATURE] SHADE reports opening / closing and tracks the move to the shade position
- [FEATURE] first command after a pause is sent without waiting for the send cycle

---

# v1.22.5

## what's new

### Commands rejected after RF noise

Continuous RF noise on the receiver could overrun the receive buffer. This overwrote adjacent memory, including the command queue. Afterwards the controller rejected every new command with "too many commands within too short time", and only a restart helped. The receive buffer is now protected against overrun.

### Command queue watchdog

As an additional safeguard, a command queue that does not shrink for more than 12 seconds is logged as stuck and reset, instead of blocking all further commands. The error "too many commands within too short time" now also shows the queue size.

## changelog

- [FIX] RF noise could overrun the receive buffer and block all further commands
- [FEATURE] watchdog resets a stuck command queue
- [FEATURE] queue size in the "too many commands" log message

---

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

---

# v1.22.3

## what's new

Fixes [issue #6](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/6).

### Remote control only moves the position of the selected shutter

When a remote was registered for several shutters (e.g. a TDR16 for 7 shutters), every button press updated the position of all shutters in its mask, no matter which channel was selected on the remote. Now only the shutters whose channel was actually sent are updated - remote channel n belongs to shutter n. Single-channel remotes that are assigned to other shutters keep updating their whole mask.

## changelog

- [FIX] Remote commands only update the position of the shutters on the selected remote channel(s)

---

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

---

# v1.22.1

## what's new

Fixes the crashes reported in [issue #5](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/5).

### ESP32-S2 no longer crashes when the WebUI is open

The ESP32-S2 build used the board definition of a classic ESP32. With it the embedded PSRAM of the S2 was never initialised, and only about 15 KB of internal RAM were left once WiFi and the web server were running. As soon as the WebUI was opened, sending the page data failed with an out-of-memory `abort()` - the ESP restarted in a loop, which also made the setup impossible.

- The S2 build now uses the `lolin_s2_mini` board definition (ESP32-S2FN4R2 with 2 MB PSRAM). PSRAM is now active and the free internal RAM rose from ~15 KB to ~65 KB.
- The WebUI log buffer (40 KB) is now allocated at startup and placed in PSRAM if available.
- Timer fields are sent to the WebUI in small packets, one channel or group at a time, and only when enough memory is free.
- On the S2 the log is now also printed on the USB port ("USB CDC On Boot"), and firmware uploads no longer need the BOOT button.

> [!NOTE]
> After this update the ESP32-S2 shows up as a different COM port, because its USB ID changed.

> [!IMPORTANT]
> The default CC1101 pins (GDO2 = GPIO22, MOSI = GPIO23) do not exist on the ESP32-S2. Set pins that exist on your board in the GPIO settings.

## changelog

- [FIX] ESP32-S2: crash loop (out of memory) as soon as the WebUI is opened - PSRAM is now used (#5)
- [FIX] Timer: "Go to position" defaults to 50 % instead of 0 % for new timers
- [CHANGE] ESP32-S2 build uses the `lolin_s2_mini` board definition with USB CDC on boot
- [CHANGE] WebUI log buffer is allocated in PSRAM if available
- [CHANGE] Timer fields are sent to the WebUI one channel/group per packet

---

# v1.22.0

## what's new

Feature request from [discussion #2](https://github.com/Banabas/ESP32-Jarolift-PositionController/discussions/2) - thanks to @ruppik.

### Timer can move shutters to a position

Every channel and group timer now has a third event **Go to position** next to UP and DOWN. It moves the shutter to a configurable target position (0 % = closed, 100 % = open), for example a shading position at noon. This works for motors that do not support the built-in shade function.

The event offers the same trigger options as UP and DOWN: fixed time, sunrise/sunset with astro mode and offset, and the weekend override.

> [!NOTE]
> Position moves need a calibrated travel time. Uncalibrated channels ignore the event and write a warning to the log. For group timers each channel in the group is moved separately.

## changelog

- [FEATURE] Timer: new "Go to position" event per channel and group timer with a target position in % (#2)

---

# v1.21.5

## what's new

Bug fix reported in [issue #8](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/8) - thanks to @Reinhard-M.

### Position 0-4 % via MQTT works

Shutter topics (`../cmd/shutter/N`) accepted the digits `0`-`4` as legacy commands (`0` = UP, `1` = DOWN, `2` = STOP, `3` = SHADE, `4` = SETSHADE). The same topic also takes a numeric target position, so sending position `0` opened the shutter instead of closing it, and positions 1-4 % never reached the position control. This also affected the Home Assistant position slider.

Shutter topics now only accept the text commands `UP`/`OPEN`, `DOWN`/`CLOSE`, `STOP`, `SHADE` and `SETSHADE`; every number from 0 to 100 is a target position.

> [!IMPORTANT]
> If you send the digits `0`-`4` as commands to `../cmd/shutter/N`, switch to the text commands. Group topics (`../cmd/group/N`) are unchanged and still accept the digits.

## changelog

- [FIX] Numeric payloads on shutter topics are always treated as target positions - `0` closes the shutter instead of opening it (#8)
- [BREAKING] Digit commands `0`-`4` are no longer accepted on `../cmd/shutter/N` - use `UP`, `DOWN`, `STOP`, `SHADE`, `SETSHADE`

---

# v1.21.4

## what's new

Bug fixes reported in [issue #1](https://github.com/Banabas/ESP32-Jarolift-PositionController/issues/1) - thanks to @airwolf-it for the detailed report. Three of them are inherited from upstream.

### Reception now works right after boot

`begin()` attached the RX interrupt but never put the CC1101 into RX state, leaving the chip in IDLE. Reception only started working once the controller had transmitted something, which enters RX as a side effect - so on a freshly booted device remote signals were silently ignored.

### The "new learn mode" switch works

Two problems kept the switch from doing anything. A copy/paste leftover from the log-level handling forced `learn_mode` back to on whenever it was stored as off, and `setLegacyLearnMode()` was never called, so the configured value never reached the controller. Both are fixed; the switch now also takes effect immediately instead of only after a restart. Receivers that need the legacy rear-button learn method (button code `0x1`) can be used again.

### Builds on Linux

`Dusk2Dawn` included `<Math.h>` instead of `<math.h>`, which fails on case-sensitive filesystems.

## changelog

- [FIX] CC1101 now enters RX state at the end of `begin()` - remote reception works on a freshly booted device
- [FIX] `learn_mode` is no longer forced back to on when saved or loaded as off
- [FIX] `setLegacyLearnMode()` is called from `jaroliftSetup()`, so the WebUI switch reaches the controller
- [FIX] `Dusk2Dawn` includes `<math.h>` in the correct case - fixes the build on Linux

---

# v1.21.3

## what's new

### Accurate position tracking for remote-controlled moves

Commands sent by a physical Jarolift remote (or a sensor acting as one) previously reset the stored position to a flat 0% / 100%, and STOP was ignored entirely. A shutter driven by hand - e.g. stopped halfway, then moved the other way - ended up with a wrong estimated position. Remote-triggered UP/DOWN/STOP now feed the same time-based position tracker that is used for the controller's own commands. Channels without a calibrated travel time keep the previous behaviour.

### More detail in the log

Start and end of a tracked move are now logged with channel name and position.

## changelog

- [FEATURE] Remote-triggered UP/DOWN/STOP update the time-based position tracker
- [FEATURE] Log start and end of tracked moves with channel name and position

---

# v1.21.2

## what's new

### Longer log history

The web log buffer now keeps 320 entries instead of 200, so more history is available before older entries scroll out (e.g. after several remote-control button presses).

## changelog

- [FEATURE] Log buffer increased from 200 to 320 entries

---

# v1.21.1

## what's new

Small follow-up fix found right after publishing v1.21.0.

### Remote signals are logged again regardless of MQTT state

`mqttSendRemote()` returned immediately if MQTT was not connected (or disabled), which skipped the "received remote signal" log line entirely - so remote button presses no longer showed up in the log unless MQTT happened to be connected at that exact moment. The log line is now always written; only the MQTT status publish is skipped when there is no broker connection.

## changelog

- [FIX] Remote-control commands are logged again even when MQTT is disabled or disconnected

---

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

---

# v1.20.0

## what's new

This is a major feature release.

> [!TIP]
> Maybe it is necessary to clean your browser cache after the update, to be sure that everything works well!

### Time-based position control

Roller shutters can now be moved to a specific position (0–100 %) directly via the WebUI slider or via MQTT.
Position tracking is purely time-based: after a one-time calibration the firmware calculates how long the motor has to run to reach the desired position and then sends an RF STOP at the right moment.

**Calibration** is done in two phases (DOWN then UP) from the service page.  
Both travel times (DOWN and UP) are stored independently so that asymmetric motors are handled correctly.

### Extended timer system

The timer has been completely redesigned:

- **Per-channel timers** – each of the 16 channels can have its own independent UP/DOWN schedule
- **Per-group timers** – each of the 6 groups can also have an independent UP/DOWN schedule
- **Weekend override** – Saturday/Sunday can use a different time or astro event than the weekday schedule
- **Astro modes** – choose between Real, Civil, Nautical, Astronomical twilight or a custom Horizon angle for sunrise/sunset events
- **Min/Max time constraints** – limit astro-based events so they never fire before a minimum or after a maximum clock time

### ESP32-S3 16 MB support

A new build target `esp32s3_16mb` with a matching 16 MB partition table has been added.

## changelog

- [FEATURE] Time-based position control via WebUI slider and MQTT numeric payload (0–100 %)
- [FEATURE] Two-phase calibration (DOWN + UP) with independent travel times per channel
- [FEATURE] Per-channel timer with individual UP/DOWN events and weekday selection
- [FEATURE] Per-group timer with individual UP/DOWN events and weekday selection
- [FEATURE] Weekend override for timer events (Sa+So use separate time/astro settings)
- [FEATURE] Astro modes: Real, Civil, Nautical, Astronomical, Horizon for sunrise/sunset timers
- [FEATURE] Min/Max time constraints for astro-based timer events
- [FEATURE] New build target esp32s3_16mb (16 MB flash with matching partition table)
- [FIX] Timer overview: `astroKeys` renamed to `astroKeyNames` – fixes ReferenceError for astro-mode timers
- [FIX] Timer overview: `buildTimerOverview()` now called after server data load instead of fixed timeout
- [FIX] Timer weekend override: corrected element ID lookup in `toggleWeekend()` (`_we_block` instead of `_we_we_block`)
- [FIX] Browser refresh: timer fields `horizon_value`, `use_min_time`, `use_max_time`, `min_time` and `max_time` now correctly restored after page reload
- [FIX] Service log: channel number in learn/unlearn/endpoint commands now correct
- [FIX] Config load: duplicate `eth.ipaddress` read removed

---

# v1.8.0

## what's new

### Timer: new option to set minimum or maximum time in combination with sunrise or sundown

Thanks to @blissi it is now possible to set additional minimum or maximum time in combination with sunrise or sundown.
You can therefore define that the shutter should open at sunrise, but not before 06:30, for example
Or close at sunset, but no later than 21:00

## changelog

- [UPDATE] dewenni/EspWebUI @ 0.0.2
- [FIX] bugfix github ota asset check #40
- [FEATURE] Timer: new option to set minimum or maximum time in combination with sunrise or sundown #41

---

# v1.7.0

## what's new

### EspWebUI-Library

with this update, the basic WebUI functions were outsourced to a separate [EspWebUI](https://github.com/dewenni/EspWebUI) library.
This should have no effect on the user experience, but it improves the handling of similar projects like [ESP_Buderus_KM271](https://github.com/dewenni/ESP_Buderus_KM271)

### support for received remote signals

As requested by some users, there is now also support for original remote controls. The signals from the remote controls are now captured, processed and also made available via MQTT. Further details can be found in the updated and extended readme.

### support for different ESP32 Chips

There is now also support for other ESP32 chips. This allows you a greater variance in the selection of your hardware.
For the newly supported chips, a firmware file for OTA updates and one for initial flashing will be offered in future. The direct GitHub update via the WebUI should also work for the new chips.

Supported Chips:

- ESP32
- ESP32-S2 (new)
- ESP32-S3 (new)
- ESP32-C3 (new)

### some minor WebUI improvements

- When a shutter command is executed in the WebUI, a message confirming the command is displayed for 2 seconds at the footer.
- Add additional ESP-Chip infos in the system section of the WebUI
- Add add additional error messages at the footer if Jarolift Keys or serial number not set

## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.6
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.7.2
- [UPDATE]  bblanchon/ArduinoJson @ 7.3.1
- [CHANGE] Basic WebUI functions were outsourced to a separate [EspWebUI](https://github.com/dewenni/EspWebUI) library
- [FIX] update WiFi and Ethernet Status also if mqtt is not connected
- [FIX] the entry of Jarolift Keys or the serial number was not accepted if you clicked on LOCK directly after the entry
- [FIX] skip websocket connection if WebUI is located on localhost or github.io
- [FEATURE] add additional error messages if Jarolift Keys or serial number not set
- [FEATURE] When a shutter command is executed in the WebUI, a message confirming the command is displayed for 2 seconds at the footer.
- [FEATURE] add support for received remote signals
- [FEATURE] add support for multiple ESP32 Chips
- [FEATURE] add additional ESP-Chip infos in the system section of the WebUI

---

# v1.6.2

## what's new

this is a minor bugfix version


## changelog

- [BUGFIX] Login problems due to faulty cookie handling fixed and improved #30

---

# v1.6.1

## what's new

this is a minor bugfix version

## changelog

- [BUGFIX] URL entries in the settings were not saved #29

---

# v1.6.0

## what's new

this is a minor update and bugfix version

### WebUI-Demo

For a first impression of the functions and the WebUI, a limited demo is also available.  
This can be accessed via the following link: [WebUI-DEMO](https://dewenni.github.io/ESP32-Jarolift-Controller/)

### Changed condition for Setup-Mode

Setup mode is now activated when the ESP is restarted **5** times.
A maximum of 5 seconds may elapse after each restart.

Example: restart 1/5 - wait 2s - restart 2/5 - wait 2s - restart 3/5 - wait 2s - restart 4/5 - wait 2s - restart /5/5 => Setup-Mode


## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.5
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.7.0
- [IMPROVE] Added validation for IP input fields in the setup area to avoid wrong syntax #27
- [IMPROVE] Conversion of the storage of the device counter from EEPROM to NVS
- [IMPROVE] remove callback function for internal messages in jarolift-controller library
- [IMPROVE] change "Double-Reset-Detection" to "Multi-Reset-Detection" now it needs 5 restarts within 5s timeout to enter Setup-Mode #26
- [NEW] Add a [WebUI-DEMO](https://dewenni.github.io/ESP32-Jarolift-Controller/) under github-pages
- [NEW] Added some description for possible teach-in procedures.

---

# v1.5.1

## what's new

this is a minor update and bugfix version

## changelog

- [UPDATE]  ESP32Async/AsyncTCP @ 3.3.2
- [UPDATE]  ESP32Async/ESPAsyncWebServer @ 3.6.2
- [UPDATE]  bblanchon/ArduinoJson @ 7.3.0
- [UPDATE]  dewenni/EspSysUtil @ 1.0.1
- [IMPROVE] add button for GitHub-OTA in Tools section #34
- [BUGFIX]  some log messages got lost at startup
- [BUGFIX]  WiFi Status information was not updated in webUI

---

# v1.5.0

## what's new

This update brings a cool new feature 🎉🎉🎉

### 1. Timer Control

The timer function enables the automatic control of individual roller shutters or a selection of several roller shutters as a group.
A fixed time can be specified as a trigger, or sunrise or sunset with an optional time offset.
This controller can therefore also work well as a stand-alone solution without the need for further automation

![webUI_timer](/Doc/webUI_timer.png)

The selection of roller shutters is supported by an additional dialogue. All configured and activated roller shutters are displayed there. These can be selected there and the bitmask is then automatically created from them.

<img style="width: 400px;" src="./Doc/webUI_bitmask_wiz.png">

> [!TIP]
> Maybe it is necessary to clean your browser cache after the update, to be sure that everything works well!

### encrypted Passwords

Passwords are now better protected and stored in encrypted form in the config.json
When updating, the existing passwords are automatically encrypted and saved again.

> [!CAUTION]
> As the passwords are stored in encrypted form after this update, it is no longer possible to switch to an older version without re-entering the passwords after the downgrade

## changelog

- [UPDATE] Arduino core 3.1.1 based on IDF 5.3.2.241224
- [UPDATE] mathieucarbou/AsyncTCP @ 3.3.2
- [UPDATE] mathieucarbou/ESPAsyncWebServer @ 3.6.0
- [FEATURE] new feature to define 6 individual timer to automatically move the shutters #4
- [FEATURE] new wizard for defining bit masks for timers and groups
- [IMPROVE] uploaded config files via "config-upload" are now automatically renamed to config.json, regardless of the original filename.
- [IMPROVE] Improved behavior when the restart button is pressed immediately after a change in the settings.
- [IMPROVE] Passwords are better protected and are stored in encrypted form

---

# v1.4.0

## what's new

### 1. direct GitHub Update 🎉🎉🎉

This update introduces an exciting new feature:  
You can now update to the latest version directly from the WebUI – no need to download the .bin file manually anymore!

Simply click on the version info in the bottom left corner, and a dialog will open. If a new version is available, you can initiate the update right there. It will automatically download and install the latest release from GitHub.

![ota-2](Doc/github_ota.gif)

### 2. new option to enable/disable WIFI

there is a new option to enable/disable WiFi. This could be useful if you use the Ethernet connection and you do not want to use the WiFi in parallel.
The depending switch to enable/disable is also in Settings > WiFi

> [!CAUTION]
> Unfortunately after the update the "enable" will be false and therefore the Controller will be activate the "Setup-Mode" after restart, because WiFi and in most cases Ethernet is now disabled. Then you have to connect to the Accesspoint, go to Settings and activate WiFi. After Reboot everything should be fine again. Sorry, but that's something I can't avoid in this case.

> [!TIP]
> Maybe it is necessary to clean your browser cache after the update, to be sure that everything works well!

## changelog

- add new feature to update directly from GitHub
- add option to enable/disable WIFI in case ETH is used (WiFi will be activated automatically in Setup-Mode) #17
- improve the size of controls at mobile layout

---

# v1.3.1

## what's new

minor changes and housekeeping

> [!NOTE]
> this project is still in an early phase.
> If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
> If there is any bug, please use the Issue function and let me know.
> If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- add group commands for telnet interface
- code cleanup (reorganize internal functions and libraries)
- add german description
- bugfix "Build fails on build_webui.py script with FileNotFoundError" #11

---

# v1.3.0

## what's new

This release brings group support for roller shutters. It is now possible to predefine up to 6 shutter groups. These groups can then be controlled with a command via the WebUI, MQTT or Home Assistant.
It is also possible to use a generic MQTT command to specify the selection of roller shutters for a group directly as a bitmask

### predefined Group

To control shutters a group you can use the following mqtt commands.
{UP, OPEN, 0} means, that you can use one of the listed payload commands.

```text

command:    group up
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {UP, OPEN, 0}

command:    group down
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {DOWN, CLOSE, 1}

command:    group stop
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {STOP, 2}

command:    group shade
topic:      ../cmd/group/1 ... cmd/group/6
payload:    {SHADE, 3}

```

### Group with bitmask

You can also use a generic group command and provide the bitmask to select the shutters directly.  
The bitmask is a 16-bit number, with the least significant bit (on the right) representing channel 1.  
A set bit means that the channel belongs to this group.  

**Example**: `0000000000010101` means that channels 1, 3, and 5 belong to this group.

As payload, you can use three different formats to represent the same bitmask:

- **Binary**: `0b0000000000010101`
- **Hex**: `0x15`
- **Decimal**: `21`

```text

command:    group up
topic:      ../cmd/group/up
payload:    {0b0000000000010101, 0x15, 21}

command:    group down
topic:      ../cmd/group/down
payload:    {0b0000000000010101, 0x15, 21}

command:    group stop
topic:      ../cmd/group/stop
payload:    {0b0000000000010101, 0x15, 21}

command:    group shade
topic:      ../cmd/group/shade
payload:    {0b0000000000010101, 0x15, 21}

```

> [!NOTE]
> this project is still in an early phase.
> If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
> If there is any bug, please use the Issue function and let me know.
> If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- [FEATURE] highlight active page in the sidebar navigation
- [FEATURE] Implement Group Commands #6
- [FEATURE] HASS: ignore cover state (use optimistic=true) #8

---

# v1.2.0

## what's new

Implemented a command queue for incomming mqtt messages and also for the CC1101 commands.
Commands are now buffered and processed sequentially to prevent timing issues during transmission.

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- fix missing SW-Version in MQTT-SysInfo and Home Assistant
- Implemented a command queue for incoming mqtt messages. #5
- Implemented a command queue for CC1101 to handle rapid consecutive commands to prevent timing issues during transmission. #5
- update to Arduino core 3.1.0 based on IDF 5.3.2.241210

---

# v1.1.1

## what's new

this is a minor bug fix version!

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- extend internal check if cc1101 is connected to avoid infinite-loop #2


---

# v1.1.0

## what's new

minor changes and updates!

this project is still in an early phase.
If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.
If there is any bug, please use the Issue function and let me know.
If you have any further questions, suggestions or requests, please use the discussion function.

## changelog

- add gpio check (check for duplicates and set initial values if gpio=0)
- set initial log level to "Debug"
- update mathieucarbou/AsyncTCP @ 3.3.1
- update mathieucarbou/ESPAsyncWebServer @ 3.4.3

---

# v1.0.0

## what's new

this is the initial release of this new project 🎉🎉🎉

If you like this project, feel free to push the **[Star ⭐️]** button and click **[Watch 👁]** to stay updated.

If there is any bug, please use the Issue function and let me know.

If you have any further questions, suggestions or requests, please use the discussion function.
