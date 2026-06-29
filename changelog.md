**Changelog**

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
