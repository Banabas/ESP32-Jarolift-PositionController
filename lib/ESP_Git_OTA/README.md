# ESP Git OTA

This is Over The Air update library for `ESP32` that uses `GitHub` releases as the `firmware` or `SPIFFS` source.

## 📄Table of Contents

- 🏗️ [Installation](#%EF%B8%8Finstallation)
- 📝 [Usage](#usage)
- ➕ [Methods](#methods)
  - 🎉 [init GithubReleaseOTA Object](#init-githubreleaseota-object)
    - 🚨[Token hint](#token-hint)
  - 🔒️ [Setup CA Certificate](#%EF%B8%8Fsetup-ca-certificate)
  - 🏷️ [Get Tag](#%EF%B8%8Fget-tag)
  - 🔖 [Get Release](#get-release-githubrelease-object)
  - 📦️ [Get Asset](#%EF%B8%8Fget-asset-githubreleaseasset-object)
  - ⚡️ [Flash Firmware or SPIFFS](#%EF%B8%8Fflash-firmware-or-spiffs)
  - ♻️ [Free Memory](#%EF%B8%8Ffree-memory)
- 👽️ [Object](#%EF%B8%8Fobject)
  - [GithubRelease](#githubrelease)
  - [GithubReleaseAsset](#githubreleaseasset)
  - [GithubAuthor](#githubauthor)

## 🏗️Installation

- For PlatformIO:  

Add the following line to your `platformio.ini` file:

```ini
; Use PlatformIO repository
lib_deps =
    hsun/ESP_Git_OTA
  ; hsun/ESP32_Git_OTA ; For Old version

; Use GitHub repository
lib_deps =
    https://github.com/Hsun1031/ESP_Git_OTA.git
```

## 📝Usage

```cpp
#include <Arduino.h>
#include <ESP_Git_OTA.h>

#define ESP_VERSION "v1.0.0"

GithubRelease release;
bool readyForUpdate = false;

// Public repository
GithubReleaseOTA ota("username", "repository");
// Private repository See Token hint https://github.com/Hsun1031/ESP32_Git_OTA?tab=readme-ov-file#token-hint
// GithubReleaseOTA ota("username", "repository", "token");

void setup() {
    Serial.begin(115200);

    // Setup WiFi connection...

    // Setup CA certificate for HTTPS connection or use empty string
    // ota.setCACert(PEM_CA_CERT);

    // Get the latest release from GitHub
    release = ota.getLatestRelease();

    // Check if the latest release is newer than the current version
    if (release.tag_name != NULL) {
        if (strcmp(release.tag_name, ESP_VERSION) == 0) {
            Serial.println("Already up to date");
            readyForUpdate = false;

            ota.freeRelease(release);
        } else {
            Serial.println("New version available: " + String(release.tag_name));
            readyForUpdate = true;
        }
    }
}

void loop() {
    if (readyForUpdate) {
        int result = ota.flashFirmware(release, "firmware.bin");
        Serial.println("Flash firmware result: " + String(result));

        if (result == 0) {
            Serial.println("Firmware updated successfully");

            ESP.restart();
        } else {
            Serial.println("Firmware update failed: " + String(result));
            ota.freeRelease(release);
            readyForUpdate = false;
        }
    } else {
        Serial.println("Failed to get latest release");
        readyForUpdate = false;
    }
}

```

## ➕Methods

### 🎉init GithubReleaseOTA Object

#### ✨`GithubReleaseOTA(const char* username, const char* repository)` Initialize GithubReleaseOTA object with public repository

- `Parameters`:
  - `username` - `const char*`: GitHub username
  - `repository` - `const char*`: GitHub repository

#### ✨`GithubReleaseOTA(const char* username, const char* repository, const char* token)` Initialize GithubReleaseOTA object with private repository

- `Parameters`:
  - `username` - `const char*`: GitHub username
  - `repository` - `const char*`: GitHub repository
  - `token` - `const char*`: GitHub token

#### 🚨`token` hint

Token need `Contents` repository permissions (read), see Github API: [Get a release asset](https://docs.github.com/en/rest/releases/assets?apiVersion=2022-11-28#get-a-release-asset)

### 🔒️Setup CA Certificate

#### ✨`void setCACert(const char* ca)` Set website CA (Certificate Authority)

- `Parameters`:
  - `ca` - `const char*`: Certificate Authority

example:

```cpp
const char* PEM_CA_CERT = \
    "-----BEGIN CERTIFICATE-----\n" \
    ...
    ...
    "-----END CERTIFICATE-----\n";

ota.setCACert(PEM_CA_CERT);
```

### 🏷️Get Tag

#### ✨`String getLatestRelease()` Get Latest release tag

- `Returns`:
  - `String`: Latest release

#### ✨`std::vector<String> getReleaseTagList()` Get all release tags

- `Returns`:
  - `std::vector<String>`: List of release tags

example:

```cpp
std::vector<String> tags = ota.getReleaseTagList()
for (String tag : tags) {
    Serial.println(tag);
}
```

### 🔖Get Release ([`GithubRelease`](#githubrelease) Object)

#### ✨`GithubRelease getLatestRelease()` Get Latest release

- `Returns`:
  - [`GithubRelease`](#githubrelease): Latest release

#### ✨`GithubRelease getReleaseByTagName(const char* tagName)` Get release by tag name

- `Parameters`:
  - `tagName` - `const char*`: Tag name
- `Returns`:
  - [`GithubRelease`](#githubrelease): Release

### 📦️Get Asset ([`GithubReleaseAsset`](#githubreleaseasset) Object)

#### ✨`GithubReleaseAsset getAssetByname(GithubRelease release, const char* name)` Get asset by asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name
- `Returns`:
  - [`GithubReleaseAsset`](#githubreleaseasset): Asset

### ⚡️Flash Firmware or SPIFFS

To Flash Firmware or SPIFFS, Use on `Loop` function

- Returns `int`:
  - `0`: Success
  - `1`: Null URL
  - `2`: Connect error
  - `3`: Begin error
  - `4`: Write error
  - `5`: End error

#### ✨`int flashFirmware(GithubReleaseAsset asset);` Flash firmware by asset

- `Parameters`:
  - `asset` - [`GithubReleaseAsset`](#githubreleaseasset): Asset

#### ✨`int flashFirmware(GithubRelease release, const char* name)` Flash firmware by release and asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name

#### ✨`int flashSpiffs(GithubReleaseAsset asset)` Flash SPIFFS by asset

- `Parameters`:
  - `asset` - [`GithubReleaseAsset`](#githubreleaseasset): Asset

#### ✨`int flashSpiffs(GithubRelease release, const char* name)` Flash SPIFFS by release and asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name

#### ✨`int GithubReleaseOTA::flashByAssetId(int assetId, int flashType)` Flash by asset id

- `Parameters`:
  - `assetId` - `int`: Asset id
  - `flashType` - `int`: Flash type (`Firmware`: U_FLASH, `SPIFFS`: U_SPIFFS)

### ♻️Free Memory

- ✨`void freeRelease(GithubRelease& release)` Free release object
- ✨`void freeReleaseAsset(GithubReleaseAsset& asset)` Free asset object
- ✨`void freeAuthor(GithubAuthor& author)` Free author object

## 👽️Object

See Github API for more information: [GitHub API](https://docs.github.com/en/rest/releases/releases?apiVersion=2022-11-28)

### GithubRelease

- `url`: const char*
- `assets_url`: const char*
- `upload_url`: const char*
- `html_url`: const char*
- `id`: int
- `node_id`: const char*
- `tag_name`: const char*
- `target_commitish`: const char*
- `name`: const char*
- `draft`: bool
- `prerelease`: bool
- `created_at`: const char*
- `published_at`: const char*
- `author`: std::vector\<[GithubAuthor](#githubauthor)\>
- `assets`: std::vector\<[GithubReleaseAsset](#githubreleaseasset)\>

### GithubReleaseAsset

- `url`: const char*
- `browser_download_url`: const char*
- `id`: int
- `node_id`: const char*
- `name`: const char*
- `label`: const char*
- `content_type`: const char*
- `state`: const char*
- `size`: int
- `download_count`: int
- `created_at`: const char*
- `updated_at`: const char*
- `author`: std::vector\<[GithubAuthor](#githubauthor)\>

### GithubAuthor

- `login`: const char*
- `id`: int
- `node_id`: const char*
- `avatar_url`: const char*
- `gravatar_id`: const char*
- `url`: const char*
- `html_url`: const char*
- `followers_url`: const char*
- `following_url`: const char*
- `gists_url`: const char*
- `starred_url`: const char*
- `subscriptions_url`: const char*
- `organizations_url`: const char*
- `repos_url`: const char*
- `events_url`: const char*
- `received_events_url`: const char*
- `type`: const char*
- `site_admin`: bool
