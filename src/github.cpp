#include <Arduino.h>
#include <basics.h>
#include <github.h>
#include <esp_heap_caps.h>
#include <message.h>
#include <mbedtls/platform.h>

#define GITHUB_OWNER "Banabas"
#define GITHUB_REPO "ESP32-Jarolift-PositionController"

static const char *TAG = "GITHUB"; // LOG TAG

GithubReleaseOTA ota(GITHUB_OWNER, GITHUB_REPO);

/**
 * *******************************************************************
 * @brief   let mbedTLS use PSRAM for its large buffers
 * @details The Arduino core forces mbedTLS into internal RAM. The TLS
 *          record buffer needs ~16 KB in one block, which the internal
 *          RAM of the ESP32-S2 usually can't provide - the GitHub
 *          request then fails with "connection refused".
 * *******************************************************************/
static void *tlsCalloc(size_t n, size_t size) {
  void *p = nullptr;
  if (n * size >= 4096) {
    p = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (p == nullptr) {
    p = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  return p;
}

static void tlsFree(void *p) { heap_caps_free(p); }

static void tlsUsePsram() {
  static bool done = false;
  if (!done && psramFound()) {
    mbedtls_platform_set_calloc_free(tlsCalloc, tlsFree);
    ESP_LOGI(TAG, "mbedTLS buffers moved to PSRAM");
  }
  done = true;
}

/**
 * *******************************************************************
 * @brief   check if the asset name contains the chip series
 * @param   assetName
 * @param   espSeries
 * @return  true if the asset name contains the chip series, else false
 * *******************************************************************/
bool isChipSeriesMatch(const char *assetName, const char *espSeries) {
  if (assetName == NULL || espSeries == NULL)
    return false;

  char formattedEspSeries[16];
  size_t j = 0;
  for (size_t i = 0; espSeries[i] != '\0' && j < sizeof(formattedEspSeries) - 1; i++) {
    if (espSeries[i] != '-') { // remove '-' from chip series
      formattedEspSeries[j++] = tolower((unsigned char)espSeries[i]);
    }
  }
  formattedEspSeries[j] = '\0';

  // check if the asset name contains the chip series anywhere (not just at start)
  return (strcasestr(assetName, formattedEspSeries) != NULL);
}

/**
 * *******************************************************************
 * @brief   set the progress callback
 * @param   callback
 * @return  none
 * *******************************************************************/
void ghSetProgressCallback(void (*callback)(int)) { ota.setProgressCallback(callback); }

/**
 * *******************************************************************
 * @brief   get the latest release from GitHub
 * @param   release
 * @param   info
 * @param   espSeries
 * @return  true if successful, else false
 * *******************************************************************/
bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info, const char *espSeries) {

  if (release == nullptr || info == nullptr) {
    return false;
  }

  // Get the latest release from GitHub
  tlsUsePsram();
  *release = ota.getLatestRelease();

  // check if the release is valid
  if (release->tag_name == nullptr || release->html_url == nullptr) {
    return false;
  }

  snprintf(info->tag, sizeof(info->tag), "%s", release->tag_name);
  snprintf(info->url, sizeof(info->url), "%s", release->html_url);
  ESP_LOGD(TAG, "GitHub latest Release: %s", info->tag);

  // search for the first asset that contains the right "chipSeries" and "ota or UPDATE" in its name
  info->assetFound = false;
  for (const auto &asset : release->assets) {
    if (isChipSeriesMatch(asset.name, espSeries) && strcasestr(asset.name, "ota") != NULL) {
      ESP_LOGD(TAG, "GitHub OTA Asset found: %s", asset.name);
      snprintf(info->asset, sizeof(info->asset), "%s", asset.name);
      info->assetFound = true;
      break;
    }
  }

  return true;
}

/**
 * *******************************************************************
 * @brief   start the OTA update
 * @param   release
 * @param   asset
 * @return  0 if successful, else error code
 * *******************************************************************/
int ghStartOtaUpdate(GithubRelease release, const char *asset) {
  tlsUsePsram();
  int result = ota.flashFirmware(release, asset);

  if (result == 0) {
    ESP_LOGI(TAG, "Firmware updated successfully");
  } else {
    ESP_LOGE(TAG, "Firmware update failed: %i", result);
    ota.freeRelease(release);
  }
  return result;
}

/**
 * *******************************************************************
 * @brief   free the release memory
 * @param   release
 * @return  none
 * *******************************************************************/
void ghFreeRelease(GithubRelease &release) {
  ota.freeRelease(release);
  release = GithubRelease(); // drop the dangling pointers so a second free is harmless
}