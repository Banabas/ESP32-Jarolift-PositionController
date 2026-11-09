#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include <ETH.h>
#include <SPI.h>
#include <basics.h>

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#endif

/* P R O T O T Y P E S ********************************************************/
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/
s_wifi wifi; // global WiFi Informations
s_eth eth;   // global ETH Informations
SPIClass *SPI_2;
s_espInfo espInfo;

static muTimer wifiReconnectTimer = muTimer(); // timer for reconnect delay
static int wifi_retry = 0;

// periodic roaming (similar to Tasmota SetOption57): rescan and switch to a clearly stronger AP
static const uint32_t WIFI_ROAM_INTERVAL = 30UL * 60UL * 1000UL; // scan every 30 minutes
static const int WIFI_ROAM_MIN_GAIN = 8;                          // switch only if >= 8 dB stronger
static uint32_t wifiRoamLastScan = 0;
static bool wifiRoamScanRunning = false;
static const char *TAG = "SETUP"; // LOG TAG

/**
 * *******************************************************************
 * @brief   flash usage of the running firmware in percent (cached)
 * @details ESP.getSketchSize() / getFreeSketchSpace() run a complete
 *          esp_image_verify() of the running image (SHA over ~1.5 MB) on
 *          every call and use bootloader_mmap(), of which only ONE mapping
 *          may exist at a time. Called cyclically (MQTT sysinfo every 10 s)
 *          this blocked the loop and collided with the verification at the
 *          end of an OTA update ("bootloader_mmap(...) failed" -> "Could Not
 *          Activate The Firmware"). The values cannot change at runtime,
 *          so they are determined once.
 * @param   none
 * @return  flash usage in %
 * *******************************************************************/
float getFlashUsagePercent() {
  static float usage = -1.0f;
  if (usage < 0.0f) {
    uint32_t sketchSize = ESP.getSketchSize();
    uint32_t freeSpace = ESP.getFreeSketchSpace();
    usage = (freeSpace > 0) ? (float)sketchSize * 100.0f / freeSpace : 0.0f;
  }
  return usage;
}

/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup() {
  configTime(0, 0,
             config.ntp.server); // 0, 0 because we will use TZ in the next line
  setenv("TZ", config.ntp.tz,
         1); // Set environment variable with your time zone
  tzset();
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_retry = 0;
  ESP_LOGI(TAG, "Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is disconnected from configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi.connected = false;
  ESP_LOGI(TAG, "WiFi-Disconnected");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGI(TAG, "WiFi connected");
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%s", WiFi.localIP().toString().c_str());
  ESP_LOGI(TAG, "IP address: %s", wifi.ipAddress);
  ESP_LOGI(TAG, "WiFi AP: BSSID %s | channel %d | RSSI %d dBm", WiFi.BSSIDstr().c_str(), (int)WiFi.channel(), (int)WiFi.RSSI());
  wifi.connected = true;
}

/**
 * *******************************************************************
 * @brief   periodic check for a stronger access point of the same SSID
 *          (mesh / repeater) and switch to it if clearly better
 * @param   none
 * @return  none
 * *******************************************************************/
static void checkWiFiRoaming() {
  if (!wifi.connected || eth.connected) {
    wifiRoamScanRunning = false;
    wifiRoamLastScan = millis(); // restart interval after (re)connect
    return;
  }

  if (!wifiRoamScanRunning) {
    if (millis() - wifiRoamLastScan < WIFI_ROAM_INTERVAL)
      return;
    // asynchronous scan for our SSID on all channels
    if (WiFi.scanNetworks(true, false, false, 300, 0, config.wifi.ssid) == WIFI_SCAN_FAILED) {
      ESP_LOGW(TAG, "WiFi roaming: scan could not be started");
      wifiRoamLastScan = millis();
      return;
    }
    wifiRoamScanRunning = true;
    return;
  }

  int16_t n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING)
    return;

  wifiRoamScanRunning = false;
  wifiRoamLastScan = millis();
  if (n <= 0) {
    WiFi.scanDelete();
    return;
  }

  // compare scan values with scan values (same measurement method);
  // the live RSSI of the connection is only used if our AP is missing in the scan
  const int liveRssi = WiFi.RSSI();
  const String currentBssid = WiFi.BSSIDstr();
  int currentScanRssi = liveRssi;
  int bestOther = -1; // strongest OTHER access point with our SSID
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) != String(config.wifi.ssid))
      continue;
    if (WiFi.BSSIDstr(i) == currentBssid) {
      currentScanRssi = WiFi.RSSI(i);
      continue;
    }
    if (bestOther < 0 || WiFi.RSSI(i) > WiFi.RSSI(bestOther))
      bestOther = i;
  }

  // switch only if the other AP is at least WIFI_ROAM_MIN_GAIN dB STRONGER
  // (e.g. current -70 dBm, other -60 dBm -> switch; other -78 dBm -> stay)
  if (bestOther >= 0 && WiFi.RSSI(bestOther) >= currentScanRssi + WIFI_ROAM_MIN_GAIN) {
    uint8_t bssid[6];
    memcpy(bssid, WiFi.BSSID(bestOther), 6);
    int32_t channel = WiFi.channel(bestOther);
    ESP_LOGI(TAG, "WiFi roaming: switching from %s (%d dBm) to %s (%d dBm, channel %d)", currentBssid.c_str(), currentScanRssi,
             WiFi.BSSIDstr(bestOther).c_str(), (int)WiFi.RSSI(bestOther), (int)channel);
    WiFi.scanDelete();
    WiFi.begin(config.wifi.ssid, config.wifi.password, channel, bssid, true);
  } else {
    if (bestOther >= 0) {
      ESP_LOGD(TAG, "WiFi roaming: staying on %s (%d dBm, live %d dBm) - other AP %s (%d dBm) not >= %d dB stronger", currentBssid.c_str(),
               currentScanRssi, liveRssi, WiFi.BSSIDstr(bestOther).c_str(), (int)WiFi.RSSI(bestOther), WIFI_ROAM_MIN_GAIN);
    } else {
      ESP_LOGD(TAG, "WiFi roaming: staying on %s (%d dBm, live %d dBm) - no other AP found", currentBssid.c_str(), currentScanRssi, liveRssi);
    }
    WiFi.scanDelete();
  }
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void checkWiFi() {

  // Ethernet is also not connected - so we need to establish WiFi
  // keep trying as long as the AP is not reachable (e.g. WiFi switched off at night)
  if (wifiReconnectTimer.delayOnTrigger((!wifi.connected && !eth.connected), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();
    wifi_retry++;
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    // mesh (e.g. FRITZ!Repeater): scan all channels and connect to the strongest AP
    // instead of the first one found
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
    ESP_LOGI(TAG, "WiFi connection - attempt: %i", wifi_retry);
  }

  checkWiFiRoaming();
}

/**
 * *******************************************************************
 * @brief   Setup for general WiFi Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupWiFi() {

  if (setupMode) {
    // start Accesspoint for initial setup
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP32-Jarolift");
    ESP_LOGI(TAG, "WiFi Mode: AccessPoint SSID: ESP32-Jarolift / IP: http://192.168.4.1");
  } else if (config.wifi.enable) {

    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onWiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onWiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);

    // manual IP-Settings
    if (config.wifi.static_ip) {
      WiFi.config(IPAddress(config.wifi.ipaddress), IPAddress(config.wifi.gateway), IPAddress(config.wifi.subnet), IPAddress(config.wifi.dns));
    }

    // connect to configured wifi AP
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    // mesh (e.g. FRITZ!Repeater): scan all channels and connect to the strongest AP
    // instead of the first one found
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    WiFi.hostname(config.wifi.hostname);
    MDNS.begin(config.wifi.hostname);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
  }
}

void onEthEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    ESP_LOGI(TAG, "ETH Started");
    // set eth hostname here
    ETH.setHostname(config.eth.hostname);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    ESP_LOGI(TAG, "ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    eth.connected = true;
    snprintf(eth.ipAddress, sizeof(eth.ipAddress), "%s", ETH.localIP().toString().c_str());
    ESP_LOGI(TAG, "ETH Got IP: '%s'", eth.ipAddress);
    break;
  case ARDUINO_EVENT_ETH_LOST_IP:
    ESP_LOGI(TAG, "ETH Lost IP");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    ESP_LOGI(TAG, "ETH Disconnected");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    ESP_LOGI(TAG, "ETH Stopped");
    eth.connected = false;
    break;
  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   Setup for general Ethernet Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupETH() {

  pinMode(config.eth.gpio_irq, OUTPUT);
  pinMode(config.eth.gpio_rst, OUTPUT);

  Network.onEvent(onEthEvent);

  // Global SPI is used for CC1101 -> use second class and GPIO for Ethernet
  SPI_2 = new SPIClass(HSPI);
  SPI_2->begin(config.eth.gpio_sck, config.eth.gpio_miso, config.eth.gpio_mosi);
  SPI_2->setFrequency(SPI_MASTER_FREQ_8M);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, config.eth.gpio_cs, config.eth.gpio_irq, config.eth.gpio_rst, *SPI_2);

  if (config.eth.static_ip) {
    ETH.config(IPAddress(config.eth.ipaddress), IPAddress(config.eth.gateway), IPAddress(config.eth.subnet), IPAddress(config.eth.dns));
  }
}

/**
 * *******************************************************************
 * @brief   Call all basic Setup functions at once
 * @param   none
 * @return  none
 * *******************************************************************/
void basicSetup() {

  // determine flash usage once at startup (verifies the running image)
  getFlashUsagePercent();

  // check internal restart reason
  EspSysUtil::RestartReason::readLocal();

  // WiFi
  setupWiFi();

  // Ethernet
  if (config.eth.enable) {
    setupETH();
  }

  // NTP
  ntpSetup();

  /* Print chip information */
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  switch (chip_info.model) {
  case CHIP_ESP32:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32");
    espInfo.supported = true;
    break;
  case CHIP_ESP32S2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-S2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-S2");
    espInfo.supported = true;
    break;
  case CHIP_ESP32S3:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-S3");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-S3");
    espInfo.supported = true;
    break;
  case CHIP_ESP32C2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C2");
    espInfo.supported = false;
    break;
  case CHIP_ESP32C3:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C3");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C3");
    espInfo.supported = true;
    break;
  case CHIP_ESP32C6:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C6");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C6");
    espInfo.supported = false;
    break;
  case CHIP_ESP32H2:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-H2");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-H2");
    espInfo.supported = false;
    break;
  case CHIP_ESP32P4:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-P4");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-P4");
    espInfo.supported = false;
    break;
  case CHIP_ESP32C61:
    ESP_LOGI(TAG, "ESP-ChipSeries: ESP32-C61");
    snprintf(espInfo.chipSeries, sizeof(espInfo.chipSeries), "ESP32-C61");
    espInfo.supported = false;
    break;
  default:
    ESP_LOGI(TAG, "ESP-ChipSeries: Unknown");
    break;
  }

  snprintf(espInfo.chipModel, sizeof(espInfo.chipModel), ESP.getChipModel());
  ESP_LOGI(TAG, "ESP-ChipModel: %s", espInfo.chipModel);

  sniprintf(espInfo.chipRev, sizeof(espInfo.chipRev), "%d", ESP.getChipRevision());
  ESP_LOGI(TAG, "ESP-ChipRevision: %s", espInfo.chipRev);

  snprintf(espInfo.chipMhz, sizeof(espInfo.chipMhz), "%lu", ESP.getCpuFreqMHz());
  ESP_LOGI(TAG, "ESP-CpuFreq: %s", espInfo.chipMhz);

  snprintf(espInfo.flashSize, sizeof(espInfo.flashSize), "%s", EspStrUtil::formatBytes(ESP.getFlashChipSize()));
  ESP_LOGI(TAG, "ESP-FlashChipSize: %s", espInfo.flashSize);
}

/**
 * *******************************************************************
 * @brief   refresh Network Information
 * @param   none
 * @return  none
 * *******************************************************************/
void refreshNetworkInfo() {

  if (wifi.connected) {
    // check  RSSI
    wifi.rssi = WiFi.RSSI();

    // dBm to Quality:
    if (wifi.rssi <= -100)
      wifi.signal = 0;
    else if (wifi.rssi >= -50)
      wifi.signal = 100;
    else
      wifi.signal = 2 * (wifi.rssi + 100);
  } else {
    wifi.rssi = 0;
    wifi.signal = 0;
  }

  if (eth.connected) {
    eth.fullDuplex = ETH.fullDuplex();
    eth.linkUp = ETH.linkUp();
    eth.linkSpeed = ETH.linkSpeed();
  } else {
    eth.fullDuplex = false;
    eth.linkUp = false;
    eth.linkSpeed = 0;
  }
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendWiFiInfo() {

  JsonDocument wifiJSON;

  refreshNetworkInfo();

  if (wifi.connected) {
    wifiJSON["status"] = wifi.connected ? "connected" : "disconnected";
    wifiJSON["rssi"] = wifi.rssi;
    wifiJSON["signal"] = wifi.signal;
    wifiJSON["ip"] = wifi.ipAddress;
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  } else {
    wifiJSON["status"] = "disconnected";
    wifiJSON["rssi"] = "--";
    wifiJSON["signal"] = "--";
    wifiJSON["ip"] = "--";
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  }

  char sendWififJSON[255];
  serializeJson(wifiJSON, sendWififJSON);
  mqttPublish(addTopic("/wifi"), sendWififJSON, false);

  // wifi status
  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendETHInfo() {

  refreshNetworkInfo();

  JsonDocument ethJSON;
  ethJSON["ip"] = eth.ipAddress;
  ethJSON["status"] = eth.connected ? "connected" : "disconnected";
  ethJSON["link_up"] = eth.linkUp ? "active" : "inactive";
  ethJSON["link_speed"] = eth.linkSpeed;
  ethJSON["full_duplex"] = eth.linkUp ? "full-duplex" : "---";
  ethJSON["date_time"] = EspStrUtil::getDateTimeString();

  char sendEthJSON[255];
  serializeJson(ethJSON, sendEthJSON);
  mqttPublish(addTopic("/eth"), sendEthJSON, false);

  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   build sysinfo structure and send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendSysInfo() {

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  // ESP Heap and Flash usage
  char heap[10];
  snprintf(heap, sizeof(heap), "%.1f %%", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize());
  char flash[10];
  snprintf(flash, sizeof(flash), "%.1f %%", getFlashUsagePercent());

  JsonDocument sysInfoJSON;
  sysInfoJSON["uptime"] = uptimeStr;
  sysInfoJSON["restart_reason"].set((char *)EspSysUtil::RestartReason::get());
  sysInfoJSON["heap"] = heap;
  sysInfoJSON["flash"] = flash;
  sysInfoJSON["sw_version"] = VERSION;
  char sendInfoJSON[255] = {'\0'};
  serializeJson(sysInfoJSON, sendInfoJSON);
  mqttPublish(addTopic("/sysinfo"), sendInfoJSON, false);
}

/**
 * *******************************************************************
 * @brief   generate uptime message
 * @param   buffer
 * @param   bufferSize
 * @return  none
 * *******************************************************************/
void getUptime(char *buffer, size_t bufferSize) {
  static unsigned long previousMillis = 0;
  static unsigned long overflowCounter = 0;
  const unsigned long overflowThreshold = 4294967295;

  unsigned long currentMillis = millis();
  if (currentMillis < previousMillis) {
    // overflow detected
    overflowCounter++;
  }
  previousMillis = currentMillis;

  // calculate total uptime in seconds considering overflow
  unsigned long long totalSeconds = overflowCounter * (overflowThreshold / 1000ULL) + (currentMillis / 1000ULL);

  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  // use snprintf to format the output
  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02luh %02lum %02lus", days, hours, minutes, seconds);
  } else if (hours > 0) {
    snprintf(buffer, bufferSize, "%02luh %02lum %02lus", hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%lum %lus", minutes, seconds);
  }
}