<div align="center">
<img style="width: 100px;" src="./Doc/ESP32_Jarolift_Controller_Logo.svg">

<h3>ESP32-Jarolift-PositionController</h3>

<p><em>Forked from <a href="https://github.com/dewenni/ESP32-Jarolift-Controller">dewenni/ESP32-Jarolift-Controller</a> – extended with percentage-based position control</em></p>
</div>

-----

**[🇩🇪 Deutsche Version der Beschreibung](README_DE.md)**

-----

<div align="center">

[![Current Release](https://img.shields.io/github/release/Banabas/ESP32-Jarolift-PositionController.svg)](https://github.com/Banabas/ESP32-Jarolift-PositionController/releases/latest)
![GitHub Release Date](https://img.shields.io/github/release-date/Banabas/ESP32-Jarolift-PositionController)
![GitHub last commit](https://img.shields.io/github/last-commit/Banabas/ESP32-Jarolift-PositionController)
![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/Banabas/ESP32-Jarolift-PositionController/total?label=downloads%20total&color=%23f0cc59)
![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/Banabas/ESP32-Jarolift-PositionController/latest/total?label=downloads%20latest%20Release&color=%23f0cc59)

![GitHub watchers](https://img.shields.io/github/watchers/Banabas/ESP32-Jarolift-PositionController?style=social)
[![GitHub stars](https://img.shields.io/github/stars/Banabas/ESP32-Jarolift-PositionController.svg?style=social&label=Star)](https://github.com/Banabas/ESP32-Jarolift-PositionController/stargazers/)

</div>

-----

# ESP32-Jarolift-PositionController

Controlling Jarolift(TM) TDEF 433MHz radio shutters via **ESP32** and **CC1101** Transceiver Module in asynchronous mode.

> [!NOTE]
> This is a fork of the excellent original project by [dewenni](https://github.com/dewenni/ESP32-Jarolift-Controller).
> It extends the original with **time-based percentage position control** for individual roller shutters.
> All code changes in this fork were developed with the assistance of [Claude](https://claude.ai) (Anthropic AI),
> as the repository owner has no programming background.

-----

## What's new in this fork

### Percentage-based Position Control

The most significant addition in this fork is **time-based position control**.  
Each roller shutter can now be moved to a specific position between 0% (fully closed) and 100% (fully open) — directly from the web interface, via MQTT, or through Home Assistant. This matches Home Assistant's own convention.

> **How it works:** Since Jarolift motors do not report their actual position, the controller measures how long the motor takes to travel from fully open to fully closed (and back). Using this calibrated travel time, it calculates exactly when to stop the motor to reach any given position.

**Key features of the position control:**

- **0–100% position slider** in the WebUI for each channel
- **Separate calibration** for both the downward (closing) and upward (opening) travel time — because motors move at different speeds in each direction due to gravity
- **Accurate timing** — the stop command is issued at the exact millisecond the motor's RF command is transmitted, eliminating timing errors from command queuing
- **Position memory** — the last known position is saved to flash memory and restored after a reboot
- **MQTT position control** — send a number (0–100) as payload to move a shutter to that position
- **Home Assistant integration** — shutters appear as cover entities with a position slider; positions <= 25% are reported as "closed", 100% as "open"

![Position Slider](Doc/webUI_position_slider.png)

### Calibration

Before position control can be used, each channel must be calibrated once.  
The calibration measures the actual travel time of the motor in both directions.

**Calibration steps (Service page):**

1. Select the channel in the dropdown
2. Manually open the shutter completely (use the Up button)
3. Click **"Start Calibration"** — the shutter will automatically travel downward
4. When the shutter reaches the bottom, click **"Stop – Shutter closed"**
5. The shutter will now travel back up automatically
6. When fully open, click **"Stop – Shutter open"**
7. Both travel times are saved automatically

![Calibration](Doc/webUI_calibration.png)

> [!TIP]
> Calibration only needs to be done once per channel. The measured travel times survive reboots and are stored in the configuration file.

If the automatically measured travel times aren't quite accurate, they can be manually corrected per channel (in seconds) on the **Settings** page (fields "Travel time DOWN" / "Travel time UP").

### Updated Home Assistant Discovery

- Each shutter is now published as a **cover entity with a position slider** (0–100%)
- A separate **Shade button** is added for each shutter
- State logic: position <= 25% → `closed`, position < 100% → `stopped`, position = 100% → `open`
- ESP32 and HA position conventions match (0 = closed, 100 = open) — no inversion needed

### Position via MQTT

```text
command:    set shutter position (0-100%)
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {0 ... 100}   (integer, 0 = fully closed, 100 = fully open)

example:    move shutter 1 to 50%
topic:      jarolift/cmd/shutter/1
payload:    50
```

> [!IMPORTANT]
> The channel must be calibrated before position commands will work.
> Sending a position to an uncalibrated channel is silently ignored.

-----

## Features

- **Web-based User Interface (WebUI):**
  A modern, mobile-friendly interface for easy configuration and control.

- **Percentage Position Control:** *(new in this fork)*
  Move each shutter to any position from 0% to 100% via slider, MQTT or Home Assistant.

- **MQTT Support:**
  Communication and control via MQTT, a lightweight and reliable messaging protocol.

- **Home Assistant Integration:**
  Automatic device discovery via MQTT Auto Discovery, including position sliders and shade buttons.

- **Support for up to 16 Roller Shutters:**
  Control up to 16 roller shutters, all managed through the WebUI and MQTT.

- **Support for up to 6 Roller Shutter Groups:**
  Define shutter groups to control several shutters at once.

- **Timer function:**
  Each channel and group has its own timer with separate up and down events. Supports fixed time, sunrise/sunset with configurable astro mode (civil, nautical, astronomical, horizon) and optional weekend override.

-----

# Table of Contents

- [What's new in this fork](#whats-new-in-this-fork)
- [Hardware](#hardware)
  - [ESP32](#esp32)
  - [CC1101 433Mhz](#cc1101-433mhz)
  - [Optional: Ethernet Module W5500](#optional-ethernet-module-w5500)
- [Getting started](#getting-started)
  - [Platform-IO](#platform-io)
  - [ESP-Flash-Tool](#esp-flash-tool)
  - [OTA-Updates](#ota-updates)
  - [Setup-Mode](#setup-mode)
  - [Configuration](#configuration)
  - [Filemanager](#filemanager)
  - [Teach-in of roller shutters](#teach-in-of-roller-shutters)
  - [Migration](#migration)
- [WebUI](#webui)
  - [Channels](#channels)
  - [Groups](#groups)
  - [Timer](#timer)
- [MQTT](#mqtt)
  - [Commands](#commands)
  - [Status](#status)
  - [Home Assistant](#home-assistant)
- [Optional Communication](#optional-communication)
  - [WebUI-Logger](#webui-logger)
  - [Telnet](#telnet)

-----

# Hardware

You can find working setups from users of this project here: [working setups](https://github.com/Banabas/ESP32-Jarolift-PositionController/discussions/34)

## ESP32

The firmware is available for the following chips:

**Standard ESP32 (Xtensa® 32-bit LX6, 4MB Flash)**

- `ESP32-WROOM-32 series` (e.g. WROOM, WROOM-32D, WROOM-32U)
- `ESP32-WROVER series` (e.g. WROVER, WROVER-B, WROVER-IE)
- `ESP32-MINI series`
- `ESP32-S2 series`
- `ESP32-S3 series`

**Not compatible:**

- `ESP32-H series`
- `YB-ESP32-S3-ETH`
- `WT32-ETH01`

## CC1101 433Mhz

**Compatible and tested products:**

- `EBYTE E07-M1101D-SMA V2.0`
- `CC1101 433MHZ Green`

Standard SPI GPIO configuration:

| CC1101-Signal | ESP-GPIO |
|---------------|----------|
| VCC           | --       |
| GND           | --       |
| GD0           | 21       |
| GD2           | 22       |
| SCK/CLK       | 18       |
| MOSI          | 23       |
| MISO          | 19       |
| CS(N)         | 5        |

<img style="width: 500px;" src="./Doc/ESP32_CC1101_Steckplatine.png">

<img style="width: 500px;" src="./Doc/ESP32_CC1101_Schaltplan.png">

<img style="width: 500px;" src="./Doc/hw_1.png">

Example with ESP32-Mini and CC1101

<img style="width: 500px;" src="./Doc/hw_2.png">

Example for direct replacement with ESP32-Mini and the custom board from M. Maywald

## Optional: Ethernet Module W5500

It is also possible to connect a W5500 Ethernet module to the board or a generic ESP32.

**Compatible and tested products:**

- `W5500` HanRun (HR911105A)
- `W5500 Lite` HanRun (HR961160C)

> [!IMPORTANT]
> The connection cable should be as short as possible (approx 10cm)

Example for generic ESP32-Mini (Standard SPI port is used by CC1101):

| Signal | GPIO |
|--------|------|
| CLK    | 25   |
| MOSI   | 26   |
| MISO   | 27   |
| CS     | 32   |
| INT    | 33   |
| RST    | 17   |

-----

# Getting started

## Platform-IO

The software is created with [Visual Studio Code](https://code.visualstudio.com) and the [pioarduino-Plugin](https://github.com/pioarduino/pioarduino-vscode-ide).  
After installing the software you can clone the project from GitHub or download it as a ZIP and open it in PlatformIO.  
Then adapt the `upload_port` and corresponding settings in `platformio.ini` to your USB-to-serial adapter and upload the code to the ESP.

> [!NOTE]
> Python must also be installed to fully compile the project. The scripts folder contains scripts for building the web pages that are called during compilation.

## ESP-Flash-Tool

In the releases you will find pre-built binary files. If you don't want to use PlatformIO, you can use the `ESP32-Jarolift-PositionController_vX.X.X_espXX_flash.bin` file and flash it directly to the ESP. This file already contains bootloader, partitions and application merged into one image. Flash it at address `0x00`.

**Windows**  
Several tools are available to flash binaries to the ESP.  
One of them is [espressif-flash-download-tool](https://www.espressif.com/en/support/download/other-tools)

**macOS/Linux**  
Use esptool.py from the terminal:

```bash
pip install esptool
esptool.py -p <UPLOAD-PORT> write_flash 0x00 ESP32-Jarolift-PositionController_vX.X.X_esp32_flash.bin
```

## OTA-Updates

### Local Web OTA-Update

Download the OTA file from the latest release, then upload it through the WebUI under **Tools → OTA Update → Firmware**.

![ota-1](Doc/webUI_ota.gif)

### GitHub OTA-Update

Since version 1.4.0 it is possible to update directly from within the WebUI without downloading the file first. Click the version number in the bottom left corner to open the update dialog.

![ota-2](Doc/github_ota.gif)

### PlatformIO OTA-Update

Change the `upload_port` settings in `platformio.ini` to your ESP's IP address and use the wireless upload option.

## Setup Mode

Setup mode is activated when the ESP is restarted **5** times within 5 seconds each.

If the ESP goes into Setup Mode, it creates its own access point:  
📶 `"ESP32_Jarolift"`  
Connect to it and open the WebUI at **http://192.168.4.1**

## Configuration

- **WiFi** — enter your WiFi credentials
- **Ethernet W5500** — use Ethernet instead of WiFi
- **Authentication** — activate login with username and password
- **NTP Server** — time synchronisation for the timer function
- **MQTT** — activate MQTT and enter broker settings
- **GPIO** — configure the SPI pins for CC1101
- **Jarolift** — protocol-specific settings (Master Keys, Serial, Device Counter)
- **Shutter** — name and activate each of the 16 channels
- **Group** — define shutter groups
- **Remotes** — register existing Jarolift remote controls by serial number
- **Language** — German or English (affects WebUI and MQTT messages)

> [!NOTE]
> All settings are saved automatically when changed.

> [!IMPORTANT]
> Changes to GPIO or Jarolift settings require a restart.

![webui-settings](Doc/webUI_settings.png)

## Filemanager

The built-in file manager allows you to download and upload the `config.json` configuration file for backup and restore.

![filemanager](Doc/webUI_tools.png)

## Teach-in of roller shutters

### Teach-in via the motor button

Every TDEF motor has a programming button. Press it, then press the **Learn** button in the WebUI settings within 5 seconds.

> [!TIP]
> If the button is unreachable, briefly switch off the motor's power supply instead.

### Programming by copying an existing remote

Press UP and DOWN simultaneously on the programmed transmitter, then press STOP eight times. The motor will confirm with a vibration. Then press **Learn** in the WebUI within 5 seconds.

## Migration

Migration from [madmartin/Jarolift_MQTT](https://github.com/madmartin/Jarolift_MQTT) is possible. Match the Master Keys, Serial Number and Device Counter from the old setup. See the original project documentation for detailed steps.

-----

# WebUI

The WebUI is responsive and offers both desktop and mobile layouts.

![webui_dash](Doc/webUI_1.png)

<img style="display:inline;margin-right:50px;width:200px;" src="./Doc/webUI_mobile_1.png">
<img style="display:inline;margin-right:50px;width:200px;" src="./Doc/webUI_mobile_2.png">

## Channels

Each enabled shutter can be controlled with Up / Stop / Down / Shade buttons.  
With position control calibrated, a **position slider (0–100%)** is also available for each channel.

![webUI_shutter](Doc/webUI_shutter.png)

## Groups

Groups configured in the settings can be controlled the same way as individual shutters.

![webUI_groups](Doc/webUI_groups.png)

## Timer

Each channel and group has its own timer with separate **up** and **down** events and configurable weekdays.

Trigger options:

- **Fixed time** — specify an exact time (HH:MM)
- **Sunrise / Sunset** — with optional time offset and astro mode:
  - Real sunrise/sunset
  - Civil twilight
  - Nautical twilight
  - Astronomical twilight
  - Custom horizon angle
- **Min/Max time** — limit astro-based triggers to a time window (e.g. open at sunrise, but not before 06:30)
- **Weekend override** — separate settings for Saturday and Sunday

![webUI_timer](Doc/webUI_timer.png)

-----

# MQTT

## Commands

### Shutter

```text
command:    shutter up
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {UP, OPEN, 0}

command:    shutter down
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {DOWN, CLOSE, 1}

command:    shutter stop
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {STOP, 2}

command:    shutter shade
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {SHADE, 3}

command:    set position (0-100%)  <- new in this fork
topic:      ../cmd/shutter/1 ... cmd/shutter/16
payload:    {0 ... 100}
```

### Predefined Group

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

```text
command:    group up/down/stop/shade with bitmask
topic:      ../cmd/group/up  (or /down, /stop, /shade)
payload:    {0b0000000000010101, 0x15, 21}
```

`0000000000010101` = channels 1, 3 and 5.

## Status

```text
Status:     position (0-100%)
topic:      ../status/shutter/1 ... status/shutter/16
payload:    {0 ... 100}
            0   = fully closed
            100 = fully open
```

> [!IMPORTANT]
> With position control active, the status reflects the **estimated position** based on calibrated travel time — not feedback from the motor itself. If the shutter is operated via the original remote control, the position estimate will drift until the shutter is moved to a known end position (0% or 100%) again.

### Remotes

```json
topic:   "../status/remote/<serial-number>"
payload: {
           "name":  "<alias-name>",
           "cmd":   "<UP, DOWN, STOP, SHADE>",
           "chBin": "<channel-binary>",
           "chDec": "<channel-decimal>"
         }
```

## Home Assistant

MQTT Auto Discovery automatically registers all enabled shutters as **cover** entities in Home Assistant.

- **Position slider** — move shutters to any position 0–100%
- **Shade button** — separate button per shutter for the shade command
- **State logic** — position <= 25% shown as closed, 100% as open

<img src="Doc/webUI_ha2.png" alt="Home Assistant integration" width="75%">

<img src="Doc/webUI_ha1.png" alt="MQTT settings" width="50%">

-----

# Optional Communication

## WebUI-Logger

Log function with configurable filter levels, displayed in the WebUI.

<img src="./Doc/webUI_Logger.png" width="75%">

## Telnet

Telnet interface for commands and diagnostics:

```
telnet 192.168.178.193
```

<img src="./Doc/telnet.png" width="75%">

-----

# System

![webUI_system](Doc/webUI_system.png)

-----

# Credits and Acknowledgements

- Original project: [dewenni/ESP32-Jarolift-Controller](https://github.com/dewenni/ESP32-Jarolift-Controller)
- Original control code: Steffen Hille (2017) — [Project Home](http://www.bastelbudenbuben.de/2017/04/25/protokollanalyse-von-jarolift-tdef-motoren/)
- Based on ideas from: [madmartin/Jarolift_MQTT](https://github.com/madmartin/Jarolift_MQTT)
- Position control extension: developed with [Claude AI](https://claude.ai) (Anthropic)

-----

> Experimental version. Use at your own risk. For private/educational use only.  
> Keeloq algorithm licensed only to TI Microcontrollers.  
> This project is not affiliated with the vendor of Jarolift components.  
> Jarolift is a Trademark of Schöneberger Rolladenfabrik GmbH & Co. KG.
