Import("env")
import os
import re
import shutil
import subprocess

pioenv       = env.get("PIOENV")
BOARD_CONFIG = env.BoardConfig()
PROJECT_NAME = "ESP32-Jarolift-PositionController"

APP_BIN      = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN   = "$BUILD_DIR/${PROGNAME}_merged.bin"
RELEASE_PATH = os.path.join(env.subst("$PROJECT_DIR"), "release")


def extract_version():
    config_path = env.subst("$PROJECT_DIR/include/config.h")
    if not os.path.exists(config_path):
        return "unknown"
    with open(config_path, "r") as f:
        m = re.search(r'#define VERSION\s+"([^"]+)"', f.read())
        return m.group(1) if m else "unknown"


def write_readme(release_path, version):
    """Create README.txt with flashing instructions for all supported build targets."""
    # (build env name, esptool --chip value, description)
    targets = [
        ("esp32",        "esp32",   "ESP32-WROOM, ESP32-WROVER, ESP32-MINI"),
        ("esp32s2",      "esp32s2", "ESP32-S2"),
        ("esp32s3",      "esp32s3", "ESP32-S3 (4 MB flash)"),
        ("esp32s3_16mb", "esp32s3", "ESP32-S3 (16 MB flash)"),
    ]
    file_list = ""
    chip_list = ""
    for env_name, chip, desc in targets:
        chip_list += f"  {env_name:<14} - {desc}\n"
        file_list += f"""
  {PROJECT_NAME}_{version}_{env_name}_flash.bin
    -> Erstmaliges Flashen fuer {desc}
    -> esptool.py --chip {chip} --baud 460800 write_flash 0x0 {PROJECT_NAME}_{version}_{env_name}_flash.bin

  {PROJECT_NAME}_{version}_{env_name}_ota.bin
    -> OTA-Update fuer {desc} via WebUI -> Tools -> OTA Update
"""
    content = f"""{PROJECT_NAME} {version}
{'=' * (len(PROJECT_NAME) + len(version) + 1)}

Unterstuetzte Chips
-------------------
{chip_list}
Dateien in diesem Release
--------------------------
{file_list}
Positionssteuerung
------------------
Dieser Build enthaelt zeitbasierte Positionssteuerung (0-100%) fuer Jarolift TDEF Rolllaeden.
0% = geschlossen, 100% = offen (entspricht der Home Assistant Konvention).
Kalibrierung erforderlich pro Kanal (Service-Seite -> Laufzeit-Kalibrierung).

Quellcode
---------
https://github.com/Banabas/ESP32-Jarolift-PositionController
"""
    with open(os.path.join(release_path, "README.txt"), "w", encoding="utf-8") as f:
        f.write(content)


def merge_bin(source, target, env):
    pythonexe        = env.subst("$PYTHONEXE")
    mcu              = BOARD_CONFIG.get("build.mcu", "esp32")
    flash_size       = BOARD_CONFIG.get("upload.flash_size", "4MB")
    merged_bin_path  = env.subst(MERGED_BIN)

    flash_images     = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + ["$ESP32_APP_OFFSET", APP_BIN]
    flash_images_sub = [env.subst(x) for x in flash_images]

    cmd = [
        pythonexe, env.subst("$OBJCOPY"),
        "--chip", mcu,
        "merge_bin",
        "--flash_size", flash_size,
        "-o", merged_bin_path,
    ] + flash_images_sub

    print(f"[build_release] Merging binaries for {mcu}...")
    ret = subprocess.call(cmd)
    if ret != 0:
        print(f"[build_release] merge_bin failed (exit {ret})")
        return

    # Clear release folder only on first env to avoid stale files
    if pioenv == "esp32":
        if os.path.exists(RELEASE_PATH):
            shutil.rmtree(RELEASE_PATH)
        os.makedirs(RELEASE_PATH)
    else:
        os.makedirs(RELEASE_PATH, exist_ok=True)

    version    = extract_version()
    # Use the PlatformIO environment name (not the chip family) so that targets
    # sharing the same MCU but a different flash size/partition table - e.g.
    # esp32s3 (4MB) vs esp32s3_16mb (16MB) - don't overwrite each other's binary.
    flash_name = f"{PROJECT_NAME}_{version}_{pioenv}_flash.bin"
    ota_name   = f"{PROJECT_NAME}_{version}_{pioenv}_ota.bin"

    shutil.copyfile(merged_bin_path,  os.path.join(RELEASE_PATH, flash_name))
    shutil.copyfile(env.subst(APP_BIN), os.path.join(RELEASE_PATH, ota_name))

    # Write README only once after first env
    if pioenv == "esp32":
        write_readme(RELEASE_PATH, version)

    print(f"[build_release] Created: {flash_name}, {ota_name}")


env.AddPostAction(APP_BIN, merge_bin)
