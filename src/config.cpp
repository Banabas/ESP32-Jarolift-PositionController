#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <basics.h>
#include <config.h>
#include <message.h>

/* D E C L A R A T I O N S ****************************************************/

/**
 * V1: Initial version of dewenni.
 * V2: Added min/max times for timers.
 */
const int CFG_VERSION = 3;

char filename[24] = {"/config.json"};
bool setupMode;
bool configInitDone = false;
unsigned long hashOld;
s_config config;
muTimer checkTimer = muTimer(); // timer to refresh other values
static const char *TAG = "CFG"; // LOG TAG
char encrypted[256] = {0};
char decrypted[128] = {0};
const unsigned char key[16] = {0x6d, 0x79, 0x5f, 0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x5f, 0x6b, 0x65, 0x79, 0x31, 0x32, 0x33};

/* P R O T O T Y P E S ********************************************************/
void checkGPIO();
void configInitValue();
void configFinalCheck();

/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup() {
  // start Filesystem
  if (LittleFS.begin(true)) {
    ESP_LOGI(TAG, "LittleFS successfully started");
  } else {
    ESP_LOGE(TAG, "LittleFS error");
  }

  // load config from file
  configLoadFromFile();
  // check gpio
  checkGPIO();
  // check final settings
  configFinalCheck();
}

/**
 * *******************************************************************
 * @brief   check configured gpio
 * @param   none
 * @return  none
 * *******************************************************************/
#define MAX_GPIO 20
void checkGPIO() {
  short int usedGPIOs[MAX_GPIO];
  short int usedCount = 0;

  auto isDuplicate = [&usedGPIOs, &usedCount](int gpio) {
    if (gpio == -1)
      return false; // -1 ignore
    for (int i = 0; i < usedCount; ++i) {
      if (usedGPIOs[i] == gpio) {
        return true;
      }
    }
    if (usedCount < MAX_GPIO) {
      usedGPIOs[usedCount++] = gpio;
    }
    return false;
  };

  bool invalidcc1101 = false;
  bool invalidETH = false;

  if (config.gpio.led_setup == 0) {
    config.gpio.led_setup = LED_BUILTIN;
  } else if (isDuplicate(config.gpio.led_setup)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (led_setup)", config.gpio.led_setup);
  }

  // CC1101 GPIO
  if (config.gpio.gdo0 == 0) {
    config.gpio.gdo0 = 21;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.gdo0)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.gdo0)", config.gpio.gdo0);
  }

  if (config.gpio.gdo2 == 0) {
    config.gpio.gdo2 = 22;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.gdo2)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.gdo2)", config.gpio.gdo2);
  }

  if (config.gpio.cs == 0) {
    config.gpio.cs = 5;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.cs)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.cs)", config.gpio.cs);
  }

  if (config.gpio.sck == 0) {
    config.gpio.sck = 18;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.sck)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.sck)", config.gpio.sck);
  }

  if (config.gpio.miso == 0) {
    config.gpio.miso = 19;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.miso)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.miso)", config.gpio.miso);
  }

  if (config.gpio.mosi == 0) {
    config.gpio.mosi = 23;
    invalidcc1101 = true;
  } else if (isDuplicate(config.gpio.mosi)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (gpio.mosi)", config.gpio.mosi);
  }

  // Ethernet GPIO
  if (config.eth.gpio_cs == 0) {
    config.eth.gpio_cs = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_cs)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_cs)", config.eth.gpio_cs);
  }

  if (config.eth.gpio_irq == 0) {
    config.eth.gpio_irq = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_irq)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_irq)", config.eth.gpio_irq);
  }

  if (config.eth.gpio_miso == 0) {
    config.eth.gpio_miso = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_miso)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_miso)", config.eth.gpio_miso);
  }

  if (config.eth.gpio_mosi == 0) {
    config.eth.gpio_mosi = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_mosi)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_mosi)", config.eth.gpio_mosi);
  }

  if (config.eth.gpio_rst == 0) {
    config.eth.gpio_rst = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_rst)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_rst)", config.eth.gpio_rst);
  }

  if (config.eth.gpio_sck == 0) {
    config.eth.gpio_sck = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_sck)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_sck)", config.eth.gpio_sck);
  }

  if (config.eth.enable && invalidETH) {
    ESP_LOGE(TAG, "invalid GPIO settings for Ethernet");
  }
  if (invalidcc1101) {
    ESP_LOGE(TAG, "invalid GPIO settings for CC1101");
  }
}

/**
 * *******************************************************************
 * @brief   init hash value
 * @param   none
 * @return  none
 * *******************************************************************/
void configHashInit() {
  hashOld = EspStrUtil::hash(&config, sizeof(s_config));
  configInitDone = true;
}

/**
 * *******************************************************************
 * @brief   cyclic function for configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configCyclic() {

  if (checkTimer.cycleTrigger(1000) && configInitDone) {
    unsigned long hashNew = EspStrUtil::hash(&config, sizeof(s_config));
    if (hashNew != hashOld) {
      hashOld = hashNew;
      configSaveToFile();
    }
  }
}

/**
 * *******************************************************************
 * @brief   intitial configuration values
 * @param   none
 * @return  none
 * *******************************************************************/
void configInitValue() {

  memset((void *)&config, 0, sizeof(config));

  // Logger
  config.log.level = 4;

  // Jarolift: new learn method (button code 0xA) by default
  config.jaro.learn_mode = true;

  // WiFi
  config.wifi.enable = true;
  snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "ESP32-Jarolift");

  // MQTT
  config.mqtt.port = 1883;
  config.mqtt.enable = false;
  snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");
  snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "jarolift");

  // NTP
  snprintf(config.ntp.server, sizeof(config.ntp.server), "de.pool.ntp.org");
  snprintf(config.ntp.tz, sizeof(config.ntp.tz), "CET-1CEST,M3.5.0,M10.5.0/3");
  config.ntp.enable = true;

  // language
  config.lang = 0;

  // gpio
  config.gpio.gdo0 = 21;
  config.gpio.gdo2 = 22;
  config.gpio.sck = 18;
  config.gpio.miso = 19;
  config.gpio.mosi = 23;
  config.gpio.cs = 5;
  config.gpio.led_setup = LED_BUILTIN;

  // timer
  for (int i = 0; i < 6; i++) {
    config.timer[i].use_min_time = false;
    config.timer[i].use_max_time = false;
    strncpy(config.timer[i].min_time_value, "", sizeof(config.timer[i].min_time_value));
    strncpy(config.timer[i].max_time_value, "", sizeof(config.timer[i].max_time_value));
  }
}

/**
 * *******************************************************************
 * @brief   save configuration to file
 * @param   none
 * @return  none
 * *******************************************************************/

// Helper: write one s_timer_event into a JsonObject
static void serializeTimerEvent(JsonObject &obj, const s_timer_event &ev) {
  obj["enable"]        = ev.enable;
  obj["type"]          = ev.type;
  obj["time_value"]    = ev.time_value;
  obj["offset_value"]  = ev.offset_value;
  obj["astro_mode"]    = ev.astro_mode;
  obj["horizon_value"] = ev.horizon_value;
  obj["use_min_time"]  = ev.use_min_time;
  obj["min_time"]      = ev.min_time_value;
  obj["use_max_time"]  = ev.use_max_time;
  obj["max_time"]      = ev.max_time_value;
  obj["we_enable"]     = ev.weekend_enable;
  obj["we_type"]       = ev.weekend_type;
  obj["we_time"]       = ev.weekend_time_value;
  obj["we_offset_value"] = ev.weekend_offset_value;
  obj["we_astro_mode"]   = ev.weekend_astro_mode;
  obj["we_horizon"]    = ev.weekend_horizon_value;
}

// Helper: read one s_timer_event from a JsonObject
static void deserializeTimerEvent(JsonObject obj, s_timer_event &ev) {
  ev.enable               = obj["enable"]        | false;
  ev.type                 = obj["type"]          | 0;
  ev.offset_value         = obj["offset_value"]  | 0;
  ev.astro_mode           = obj["astro_mode"]    | 0;
  ev.horizon_value        = obj["horizon_value"] | 0;
  ev.use_min_time         = obj["use_min_time"]  | false;
  ev.use_max_time         = obj["use_max_time"]  | false;
  ev.weekend_enable       = obj["we_enable"]     | false;
  ev.weekend_type         = obj["we_type"]       | 0;
  ev.weekend_offset_value = obj["we_offset_value"] | 0;
  ev.weekend_astro_mode   = obj["we_astro_mode"]   | 0;
  ev.weekend_horizon_value= obj["we_horizon"]    | 0;
  const char *tv = obj["time_value"] | "";
  strncpy(ev.time_value, tv, sizeof(ev.time_value) - 1);
  const char *mn = obj["min_time"] | "";
  strncpy(ev.min_time_value, mn, sizeof(ev.min_time_value) - 1);
  const char *mx = obj["max_time"] | "";
  strncpy(ev.max_time_value, mx, sizeof(ev.max_time_value) - 1);
  const char *we = obj["we_time"] | "";
  strncpy(ev.weekend_time_value, we, sizeof(ev.weekend_time_value) - 1);
}

// Helper: serialize one s_channel_timer
static void serializeChannelTimer(JsonObject &obj, const s_channel_timer &ct) {
  obj["enable"]    = ct.enable;
  obj["monday"]    = ct.monday;
  obj["tuesday"]   = ct.tuesday;
  obj["wednesday"] = ct.wednesday;
  obj["thursday"]  = ct.thursday;
  obj["friday"]    = ct.friday;
  obj["saturday"]  = ct.saturday;
  obj["sunday"]    = ct.sunday;
  JsonObject up   = obj["up"].to<JsonObject>();
  JsonObject down = obj["down"].to<JsonObject>();
  serializeTimerEvent(up,   ct.up);
  serializeTimerEvent(down, ct.down);
}

// Helper: deserialize one s_channel_timer
static void deserializeChannelTimer(JsonObject obj, s_channel_timer &ct) {
  ct.enable    = obj["enable"]    | false;
  ct.monday    = obj["monday"]    | false;
  ct.tuesday   = obj["tuesday"]   | false;
  ct.wednesday = obj["wednesday"] | false;
  ct.thursday  = obj["thursday"]  | false;
  ct.friday    = obj["friday"]    | false;
  ct.saturday  = obj["saturday"]  | false;
  ct.sunday    = obj["sunday"]    | false;
  if (obj["up"].is<JsonObject>())   deserializeTimerEvent(obj["up"].as<JsonObject>(),   ct.up);
  if (obj["down"].is<JsonObject>()) deserializeTimerEvent(obj["down"].as<JsonObject>(), ct.down);
}

void configSaveToFile() {

  JsonDocument doc; // reserviert 2048 Bytes für das JSON-Objekt

  doc["version"] = CFG_VERSION;

  doc["lang"] = (config.lang);

  doc["wifi"]["enable"] = config.wifi.enable;
  doc["wifi"]["ssid"] = config.wifi.ssid;

  if (EspStrUtil::encryptPassword(config.wifi.password, key, encrypted, sizeof(encrypted))) {
    doc["wifi"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting WiFi Password");
  }

  doc["wifi"]["hostname"] = config.wifi.hostname;
  doc["wifi"]["static_ip"] = config.wifi.static_ip;
  doc["wifi"]["ipaddress"] = config.wifi.ipaddress;
  doc["wifi"]["subnet"] = config.wifi.subnet;
  doc["wifi"]["gateway"] = config.wifi.gateway;
  doc["wifi"]["dns"] = config.wifi.dns;

  doc["eth"]["enable"] = config.eth.enable;
  doc["eth"]["hostname"] = config.eth.hostname;
  doc["eth"]["static_ip"] = config.eth.static_ip;
  doc["eth"]["ipaddress"] = config.eth.ipaddress;
  doc["eth"]["subnet"] = config.eth.subnet;
  doc["eth"]["gateway"] = config.eth.gateway;
  doc["eth"]["dns"] = config.eth.dns;
  doc["eth"]["gpio_sck"] = config.eth.gpio_sck;
  doc["eth"]["gpio_mosi"] = config.eth.gpio_mosi;
  doc["eth"]["gpio_miso"] = config.eth.gpio_miso;
  doc["eth"]["gpio_cs"] = config.eth.gpio_cs;
  doc["eth"]["gpio_irq"] = config.eth.gpio_irq;
  doc["eth"]["gpio_rst"] = config.eth.gpio_rst;

  doc["mqtt"]["enable"] = config.mqtt.enable;
  doc["mqtt"]["server"] = config.mqtt.server;
  doc["mqtt"]["user"] = config.mqtt.user;

  if (EspStrUtil::encryptPassword(config.mqtt.password, key, encrypted, sizeof(encrypted))) {
    doc["mqtt"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting mqtt Password");
  }

  doc["mqtt"]["topic"] = config.mqtt.topic;
  doc["mqtt"]["port"] = config.mqtt.port;
  doc["mqtt"]["ha_enable"] = config.mqtt.ha_enable;
  doc["mqtt"]["ha_topic"] = config.mqtt.ha_topic;
  doc["mqtt"]["ha_device"] = config.mqtt.ha_device;

  doc["ntp"]["enable"] = config.ntp.enable;
  doc["ntp"]["server"] = config.ntp.server;
  doc["ntp"]["tz"] = config.ntp.tz;

  doc["geo"]["latitude"] = config.geo.latitude;
  doc["geo"]["longitude"] = config.geo.longitude;

  doc["gpio"]["gdo0"] = config.gpio.gdo0;
  doc["gpio"]["gdo2"] = config.gpio.gdo2;
  doc["gpio"]["sck"] = config.gpio.sck;
  doc["gpio"]["miso"] = config.gpio.miso;
  doc["gpio"]["mosi"] = config.gpio.mosi;
  doc["gpio"]["cs"] = config.gpio.cs;
  doc["gpio"]["led_setup"] = config.gpio.led_setup;

  doc["auth"]["enable"] = config.auth.enable;
  doc["auth"]["user"] = config.auth.user;
  doc["auth"]["password"] = config.auth.password;

  doc["logger"]["enable"] = config.log.enable;
  doc["logger"]["level"] = config.log.level;
  doc["logger"]["order"] = config.log.order;

  doc["jaro"]["masterMSB"] = config.jaro.masterMSB;
  doc["jaro"]["masterLSB"] = config.jaro.masterLSB;
  doc["jaro"]["serial"] = config.jaro.serial;
  doc["jaro"]["learn_mode"] = config.jaro.learn_mode;

  JsonArray ch_enable = doc["jaro"]["ch_enable"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    ch_enable.add(config.jaro.ch_enable[i]);
  }

  JsonArray ch_name = doc["jaro"]["ch_name"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    ch_name.add(config.jaro.ch_name[i]);
  }

  JsonArray ch_travel_time = doc["jaro"]["ch_travel_time"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    ch_travel_time.add(config.jaro.ch_travel_time[i]);
  }

  JsonArray ch_travel_time_up = doc["jaro"]["ch_travel_time_up"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    ch_travel_time_up.add(config.jaro.ch_travel_time_up[i]);
  }

  JsonArray ch_last_pos = doc["jaro"]["ch_last_pos"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    ch_last_pos.add(config.jaro.ch_last_pos[i]);
  }

  JsonArray grp_enable = doc["jaro"]["grp_enable"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    grp_enable.add(config.jaro.grp_enable[i]);
  }

  JsonArray grp_name = doc["jaro"]["grp_name"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    grp_name.add(config.jaro.grp_name[i]);
  }

  JsonArray grp_mask = doc["jaro"]["grp_mask"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    grp_mask.add(config.jaro.grp_mask[i]);
  }

  JsonArray remote_enable = doc["jaro"]["remote_enable"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    remote_enable.add(config.jaro.remote_enable[i]);
  }

  JsonArray remote_name = doc["jaro"]["remote_name"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    remote_name.add(config.jaro.remote_name[i]);
  }

  JsonArray remote_mask = doc["jaro"]["remote_mask"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    remote_mask.add(config.jaro.remote_mask[i]);
  }

  JsonArray remote_serial = doc["jaro"]["remote_serial"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    remote_serial.add(config.jaro.remote_serial[i]);
  }

  JsonArray timers = doc["timer"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    JsonObject timer = timers.add<JsonObject>();
    timer["enable"] = config.timer[i].enable;
    timer["type"] = config.timer[i].type;
    timer["time_value"] = config.timer[i].time_value;
    timer["offset_value"]  = config.timer[i].offset_value;
    timer["astro_mode"]    = config.timer[i].astro_mode;
    timer["horizon_value"] = config.timer[i].horizon_value;
    timer["cmd"] = config.timer[i].cmd;
    timer["monday"] = config.timer[i].monday;
    timer["tuesday"] = config.timer[i].tuesday;
    timer["wednesday"] = config.timer[i].wednesday;
    timer["thursday"] = config.timer[i].thursday;
    timer["friday"] = config.timer[i].friday;
    timer["saturday"] = config.timer[i].saturday;
    timer["sunday"] = config.timer[i].sunday;
    timer["grp_mask"] = config.timer[i].grp_mask;
    timer["use_min_time"] = config.timer[i].use_min_time;
    timer["use_max_time"] = config.timer[i].use_max_time;
    timer["min_time_value"] = config.timer[i].min_time_value;
    timer["max_time_value"] = config.timer[i].max_time_value;
  }

  // per-channel timers (16 shutters)
  JsonArray ch_timers = doc["ch_timer"].to<JsonArray>();
  for (int i = 0; i < 16; i++) {
    JsonObject ct = ch_timers.add<JsonObject>();
    serializeChannelTimer(ct, config.ch_timer[i]);
  }

  // per-group timers (6 groups)
  JsonArray grp_timers = doc["grp_timer"].to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    JsonObject gt = grp_timers.add<JsonObject>();
    serializeChannelTimer(gt, config.grp_timer[i]);
  }

  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to create file");
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGE(TAG, "Failed to write to file");
  } else {
    ESP_LOGI(TAG, "config successfully saved to file: %s - Version: %i", filename, CFG_VERSION);
  }

  // Close the file
  file.close();
}

/**
 * *******************************************************************
 * @brief   load configuration from file
 * @param   none
 * @return  none
 * *******************************************************************/
void configLoadFromFile() {
  // Open file for reading
  File file = LittleFS.open(filename);

  // Allocate a temporary JsonDocument
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    ESP_LOGE(TAG, "SETUP-MODE-REASON: Failed to read file, using default configuration");
    configInitValue();
    setupMode = true;

  } else {

    config.version = doc["version"];

    config.lang = doc["lang"];

    config.wifi.enable = doc["wifi"]["enable"];
    EspStrUtil::readJSONstring(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["wifi"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.wifi.password, sizeof(config.wifi.password))) {
        // ESP_LOGD(TAG, "decrypted WiFi password: %s", config.wifi.password);
      } else {
        ESP_LOGE(TAG, "error decrypting WiFi password");
      }
    }

    EspStrUtil::readJSONstring(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
    config.wifi.static_ip = doc["wifi"]["static_ip"];
    EspStrUtil::readJSONstring(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), doc["wifi"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.wifi.subnet, sizeof(config.wifi.subnet), doc["wifi"]["subnet"]);
    EspStrUtil::readJSONstring(config.wifi.gateway, sizeof(config.wifi.gateway), doc["wifi"]["gateway"]);
    EspStrUtil::readJSONstring(config.wifi.dns, sizeof(config.wifi.dns), doc["wifi"]["dns"]);

    config.eth.enable = doc["eth"]["enable"];
    EspStrUtil::readJSONstring(config.eth.hostname, sizeof(config.eth.hostname), doc["eth"]["hostname"]);
    config.eth.static_ip = doc["eth"]["static_ip"];
    EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.eth.subnet, sizeof(config.eth.subnet), doc["eth"]["subnet"]);
    EspStrUtil::readJSONstring(config.eth.gateway, sizeof(config.eth.gateway), doc["eth"]["gateway"]);
    EspStrUtil::readJSONstring(config.eth.dns, sizeof(config.eth.dns), doc["eth"]["dns"]);
    config.eth.gpio_sck = doc["eth"]["gpio_sck"];
    config.eth.gpio_mosi = doc["eth"]["gpio_mosi"];
    config.eth.gpio_miso = doc["eth"]["gpio_miso"];
    config.eth.gpio_cs = doc["eth"]["gpio_cs"];
    config.eth.gpio_irq = doc["eth"]["gpio_irq"];
    config.eth.gpio_rst = doc["eth"]["gpio_rst"];

    config.mqtt.enable = doc["mqtt"]["enable"];
    EspStrUtil::readJSONstring(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
    EspStrUtil::readJSONstring(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["mqtt"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.mqtt.password, sizeof(config.mqtt.password))) {
        // ESP_LOGD(TAG, "decrypted mqtt password: %s", config.mqtt.password);
      } else {
        ESP_LOGE(TAG, "error decrypting mqtt password");
        config.mqtt.password[0] = '\0';
      }
    }

    EspStrUtil::readJSONstring(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
    config.mqtt.port = doc["mqtt"]["port"];
    config.mqtt.ha_enable = doc["mqtt"]["ha_enable"];
    EspStrUtil::readJSONstring(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), doc["mqtt"]["ha_topic"]);
    EspStrUtil::readJSONstring(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), doc["mqtt"]["ha_device"]);

    config.ntp.enable = doc["ntp"]["enable"];
    EspStrUtil::readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    EspStrUtil::readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);

    config.geo.latitude = doc["geo"]["latitude"];
    config.geo.longitude = doc["geo"]["longitude"];

    config.gpio.led_setup = doc["gpio"]["led_setup"];
    config.gpio.gdo0 = doc["gpio"]["gdo0"];
    config.gpio.gdo2 = doc["gpio"]["gdo2"];
    config.gpio.sck = doc["gpio"]["sck"];
    config.gpio.miso = doc["gpio"]["miso"];
    config.gpio.mosi = doc["gpio"]["mosi"];
    config.gpio.cs = doc["gpio"]["cs"];

    config.auth.enable = doc["auth"]["enable"];
    EspStrUtil::readJSONstring(config.auth.user, sizeof(config.auth.user), doc["auth"]["user"]);
    EspStrUtil::readJSONstring(config.auth.password, sizeof(config.auth.password), doc["auth"]["password"]);

    config.log.enable = doc["logger"]["enable"];
    config.log.level = doc["logger"]["level"];
    config.log.order = doc["logger"]["order"];

    config.jaro.masterMSB = doc["jaro"]["masterMSB"];
    config.jaro.masterLSB = doc["jaro"]["masterLSB"];
    config.jaro.serial = doc["jaro"]["serial"];
    config.jaro.learn_mode = doc["jaro"]["learn_mode"];

    JsonArray ch_enable = doc["jaro"]["ch_enable"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.ch_enable[i] = ch_enable[i];
    }
    JsonArray ch_name = doc["jaro"]["ch_name"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      EspStrUtil::readJSONstring(config.jaro.ch_name[i], sizeof(config.jaro.ch_name[0]), ch_name[i]);
    }

    JsonArray ch_travel_time = doc["jaro"]["ch_travel_time"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.ch_travel_time[i] = ch_travel_time[i] | 0;
    }

    JsonArray ch_travel_time_up = doc["jaro"]["ch_travel_time_up"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.ch_travel_time_up[i] = ch_travel_time_up[i] | 0;
    }

    JsonArray ch_last_pos_arr = doc["jaro"]["ch_last_pos"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.ch_last_pos[i] = ch_last_pos_arr[i] | 0;
    }
    JsonArray grp_enable = doc["jaro"]["grp_enable"].as<JsonArray>();
    for (int i = 0; i < 6; i++) {
      config.jaro.grp_enable[i] = grp_enable[i];
    }
    JsonArray grp_name = doc["jaro"]["grp_name"].as<JsonArray>();
    for (int i = 0; i < 6; i++) {
      EspStrUtil::readJSONstring(config.jaro.grp_name[i], sizeof(config.jaro.grp_name[0]), grp_name[i]);
    }
    JsonArray grp_mask = doc["jaro"]["grp_mask"].as<JsonArray>();
    for (int i = 0; i < 6; i++) {
      config.jaro.grp_mask[i] = grp_mask[i];
    }
    JsonArray remote_enable = doc["jaro"]["remote_enable"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.remote_enable[i] = remote_enable[i];
    }
    JsonArray remote_name = doc["jaro"]["remote_name"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      EspStrUtil::readJSONstring(config.jaro.remote_name[i], sizeof(config.jaro.remote_name[0]), remote_name[i]);
    }
    JsonArray remote_mask = doc["jaro"]["remote_mask"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.remote_mask[i] = remote_mask[i];
    }
    JsonArray remote_serial = doc["jaro"]["remote_serial"].as<JsonArray>();
    for (int i = 0; i < 16; i++) {
      config.jaro.remote_serial[i] = remote_serial[i];
    }

    JsonArray timers = doc["timer"].as<JsonArray>();
    for (size_t i = 0; i < timers.size() && i < 6; i++) { // Schleife über die Timer
      JsonObject timer = timers[i];
      config.timer[i].enable = timer["enable"];
      config.timer[i].type = timer["type"];
      EspStrUtil::readJSONstring(config.timer[i].time_value, sizeof(config.timer[i].time_value), timer["time_value"]);
      config.timer[i].offset_value = timer["offset_value"];
      config.timer[i].cmd = timer["cmd"];
      config.timer[i].monday = timer["monday"];
      config.timer[i].tuesday = timer["tuesday"];
      config.timer[i].wednesday = timer["wednesday"];
      config.timer[i].thursday = timer["thursday"];
      config.timer[i].friday = timer["friday"];
      config.timer[i].saturday = timer["saturday"];
      config.timer[i].sunday = timer["sunday"];
      config.timer[i].grp_mask = timer["grp_mask"];
      config.timer[i].use_min_time = timer["use_min_time"];
      config.timer[i].use_max_time = timer["use_max_time"];
      EspStrUtil::readJSONstring(config.timer[i].min_time_value, sizeof(config.timer[i].min_time_value), timer["min_time_value"]);
      EspStrUtil::readJSONstring(config.timer[i].max_time_value, sizeof(config.timer[i].max_time_value), timer["max_time_value"]);
    }

    // per-channel timers
    JsonArray ch_timers = doc["ch_timer"].as<JsonArray>();
    for (size_t i = 0; i < ch_timers.size() && i < 16; i++) {
      deserializeChannelTimer(ch_timers[i].as<JsonObject>(), config.ch_timer[i]);
    }

    // per-group timers
    JsonArray grp_timers = doc["grp_timer"].as<JsonArray>();
    for (size_t i = 0; i < grp_timers.size() && i < 6; i++) {
      deserializeChannelTimer(grp_timers[i].as<JsonObject>(), config.grp_timer[i]);
    }
  }

  file.close();     // Close the file (Curiously, File's destructor doesn't close the file)
  configHashInit(); // init hash value

  // save config if version is different
  if (config.version != CFG_VERSION) {
    configSaveToFile();
    ESP_LOGI(TAG, "config file was updated from version %i to version: %i", config.version, CFG_VERSION);
  } else {
    ESP_LOGD(TAG, "config file version %i was successfully loaded", config.version);
  }
}

void configFinalCheck() {

  // check network settings
  if (strlen(config.wifi.ssid) == 0 && config.wifi.enable) {
    // no valid wifi setting => start AP-Mode
    ESP_LOGW(TAG, "SETUP-MODE-REASON: no valid wifi SSID set");
    setupMode = true;
  } else if (config.wifi.enable == false && config.eth.enable == false) {
    // no network enabled => start AP-Mode
    ESP_LOGW(TAG, "SETUP-MODE-REASON: WiFi and ETH disabled");
    setupMode = true;
  }

  // check log level
  setLogLevel(config.log.level);

  // check GIO for LED
  if (config.gpio.led_setup <= 0) {
    pinMode(LED_BUILTIN, OUTPUT); // onboard LED
  } else {
    pinMode(config.gpio.led_setup, OUTPUT); // LED for Wifi-Status
  }
}