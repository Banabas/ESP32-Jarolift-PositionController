
#include <basics.h>
#include <jarolift.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>
#include <shutter_position.h>

static const char *TAG = "WEB"; // LOG TAG
uint8_t srvShutter = 0;

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/

// Process one s_timer_event field from WebUI
static void processTimerEventField(const char *elementId, const char *value,
                                    const char *prefix, s_timer_event &ev) {
  char id[48];
  snprintf(id, sizeof(id), "%s_enable",        prefix); if (strcmp(elementId, id) == 0) { ev.enable        = EspStrUtil::stringToBool(value); return; }
  snprintf(id, sizeof(id), "%s_type",          prefix); if (strcmp(elementId, id) == 0) { ev.type          = (uint8_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_time_value",    prefix); if (strcmp(elementId, id) == 0) { snprintf(ev.time_value, sizeof(ev.time_value), "%s", value); return; }
  snprintf(id, sizeof(id), "%s_offset_value",  prefix); if (strcmp(elementId, id) == 0) { ev.offset_value  = (int16_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_astro_mode",    prefix); if (strcmp(elementId, id) == 0) { ev.astro_mode    = (uint8_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_horizon_value", prefix); if (strcmp(elementId, id) == 0) { ev.horizon_value = (int8_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_use_min_time",  prefix); if (strcmp(elementId, id) == 0) { ev.use_min_time  = EspStrUtil::stringToBool(value); return; }
  snprintf(id, sizeof(id), "%s_use_max_time",  prefix); if (strcmp(elementId, id) == 0) { ev.use_max_time  = EspStrUtil::stringToBool(value); return; }
  snprintf(id, sizeof(id), "%s_min_time",      prefix); if (strcmp(elementId, id) == 0) { snprintf(ev.min_time_value, sizeof(ev.min_time_value), "%s", value); return; }
  snprintf(id, sizeof(id), "%s_max_time",          prefix); if (strcmp(elementId, id) == 0) { snprintf(ev.max_time_value, sizeof(ev.max_time_value), "%s", value); return; }
  snprintf(id, sizeof(id), "%s_we_enable",          prefix); if (strcmp(elementId, id) == 0) { ev.weekend_enable        = EspStrUtil::stringToBool(value); return; }
  snprintf(id, sizeof(id), "%s_we_type",            prefix); if (strcmp(elementId, id) == 0) { ev.weekend_type          = (uint8_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_we_time_value",      prefix); if (strcmp(elementId, id) == 0) { snprintf(ev.weekend_time_value, sizeof(ev.weekend_time_value), "%s", value); return; }
  snprintf(id, sizeof(id), "%s_we_offset_value",    prefix); if (strcmp(elementId, id) == 0) { ev.weekend_offset_value  = (int16_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_we_astro_mode",      prefix); if (strcmp(elementId, id) == 0) { ev.weekend_astro_mode    = (uint8_t)atoi(value); return; }
  snprintf(id, sizeof(id), "%s_we_horizon_value",   prefix); if (strcmp(elementId, id) == 0) { ev.weekend_horizon_value = (int8_t)atoi(value); return; }
}

void webCallback(const char *elementId, const char *value) {

  ESP_LOGD(TAG, "Received - Element ID: %s = %s", elementId, value);

  // ------------------------------------------------------------------
  // GitHub / Version
  // ------------------------------------------------------------------

  // Github Check Version
  if (strcmp(elementId, "check_git_version") == 0 || strcmp(elementId, "p11_check_git_btn") == 0) {
    requestGitHubVersion();
  }
  // Github Update
  if (strcmp(elementId, "p00_update_btn") == 0) {
    requestGitHubUpdate();
  }
  // OTA-Confirm
  if (strcmp(elementId, "p00_ota_confirm_btn") == 0) {
    webUI.wsUpdateWebDialog("ota_update_done_dialog", "close");
    EspSysUtil::RestartReason::saveLocal("ota update");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // ------------------------------------------------------------------
  // Settings callback
  // ------------------------------------------------------------------

  // WiFi
  if (strcmp(elementId, "cfg_wifi_enable") == 0) {
    config.wifi.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_hostname") == 0) {
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_password") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_static_ip") == 0) {
    config.wifi.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_ipaddress") == 0) {
    snprintf(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_subnet") == 0) {
    snprintf(config.wifi.subnet, sizeof(config.wifi.subnet), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_gateway") == 0) {
    snprintf(config.wifi.gateway, sizeof(config.wifi.gateway), "%s", value);
  }
  if (strcmp(elementId, "cfg_wifi_dns") == 0) {
    snprintf(config.wifi.dns, sizeof(config.wifi.dns), "%s", value);
  }

  // Ethernet
  if (strcmp(elementId, "cfg_eth_enable") == 0) {
    config.eth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_hostname") == 0) {
    snprintf(config.eth.hostname, sizeof(config.eth.hostname), "%s", value);
  }
  if (strcmp(elementId, "cfg_eth_gpio_sck") == 0) {
    config.eth.gpio_sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_mosi") == 0) {
    config.eth.gpio_mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_miso") == 0) {
    config.eth.gpio_miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_cs") == 0) {
    config.eth.gpio_cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_irq") == 0) {
    config.eth.gpio_irq = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_rst") == 0) {
    config.eth.gpio_rst = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_static_ip") == 0) {
    config.eth.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_ipaddress") == 0) {
    snprintf(config.eth.ipaddress, sizeof(config.eth.ipaddress), "%s", value);
  }
  if (strcmp(elementId, "cfg_eth_subnet") == 0) {
    snprintf(config.eth.subnet, sizeof(config.eth.subnet), "%s", value);
  }
  if (strcmp(elementId, "cfg_eth_gateway") == 0) {
    snprintf(config.eth.gateway, sizeof(config.eth.gateway), "%s", value);
  }
  if (strcmp(elementId, "cfg_eth_dns") == 0) {
    snprintf(config.eth.dns, sizeof(config.eth.dns), "%s", value);
  }

  // Authentication
  if (strcmp(elementId, "cfg_auth_enable") == 0) {
    config.auth.enable = EspStrUtil::stringToBool(value);
    webUI.setAuthentication(config.auth.enable);
  }
  if (strcmp(elementId, "cfg_auth_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), "%s", value);
    webUI.setCredentials(config.auth.user, config.auth.password);
  }
  if (strcmp(elementId, "cfg_auth_password") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), "%s", value);
    webUI.setCredentials(config.auth.user, config.auth.password);
  }

  // NTP-Server
  if (strcmp(elementId, "cfg_ntp_enable") == 0) {
    config.ntp.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "%s", value);
  }

  // GEO-Location
  if (strcmp(elementId, "cfg_geo_latitude") == 0) {
    config.geo.latitude = atof(value);
  }
  if (strcmp(elementId, "cfg_geo_longitude") == 0) {
    config.geo.longitude = atof(value);
  }

  // MQTT
  if (strcmp(elementId, "cfg_mqtt_enable") == 0) {
    config.mqtt.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_password") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_enable") == 0) {
    config.mqtt.ha_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_topic") == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_device") == 0) {
    snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "%s", value);
  }

  // Hardware
  if (strcmp(elementId, "cfg_gpio_gdo0") == 0) {
    config.gpio.gdo0 = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_gdo2") == 0) {
    config.gpio.gdo2 = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_sck") == 0) {
    config.gpio.sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_miso") == 0) {
    config.gpio.miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_mosi") == 0) {
    config.gpio.mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_cs") == 0) {
    config.gpio.cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_led_setup") == 0) {
    config.gpio.led_setup = strtoul(value, NULL, 10);
  }

  // Jarolift settings
  if (strcmp(elementId, "cfg_jaro_masterMSB") == 0) {
    config.jaro.masterMSB = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_masterLSB") == 0) {
    config.jaro.masterLSB = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_serial") == 0) {
    config.jaro.serial = strtoul(value, NULL, 16);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "cfg_jaro_learn_mode") == 0) {
    config.jaro.learn_mode = EspStrUtil::stringToBool(value);
    jaroCmdReInit();
  }
  if (strcmp(elementId, "p12_jaro_devcnt") == 0) {
    jaroCmdSetDevCnt(strtoul(value, NULL, 10));
  }

  // Shutter 1-16
  for (int i = 0; i < 16; i++) {
    char enableId[32];
    char nameId[32];
    char learnId[32];
    char unlearnId[32];
    char cmdUpId[32];
    char cmdDownId[32];
    char cmdStopId[32];
    char cmdShadeId[32];

    snprintf(enableId, sizeof(enableId), "cfg_jaro_ch_enable_%d", i);
    snprintf(nameId, sizeof(nameId), "cfg_jaro_ch_name_%d", i);
    snprintf(unlearnId, sizeof(unlearnId), "p12_unlearn_%d", i);
    snprintf(learnId, sizeof(learnId), "p12_learn_%d", i);

    snprintf(cmdUpId, sizeof(cmdUpId), "p01_up_%d", i);
    snprintf(cmdDownId, sizeof(cmdDownId), "p01_down_%d", i);
    snprintf(cmdStopId, sizeof(cmdStopId), "p01_stop_%d", i);
    snprintf(cmdShadeId, sizeof(cmdShadeId), "p01_shade_%d", i);

    if (strcmp(elementId, enableId) == 0) {
      config.jaro.ch_enable[i] = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, nameId) == 0) {
      snprintf(config.jaro.ch_name[i], sizeof(config.jaro.ch_name[i]), "%s", value);
    }

    // Manual travel-time correction (entered in seconds, stored internally in ms)
    char traveltimeId[40];
    char traveltimeUpId[40];
    snprintf(traveltimeId, sizeof(traveltimeId), "cfg_jaro_ch_traveltime_%d", i);
    snprintf(traveltimeUpId, sizeof(traveltimeUpId), "cfg_jaro_ch_traveltime_up_%d", i);
    if (strcmp(elementId, traveltimeId) == 0) {
      uint32_t ms = (uint32_t)(atof(value) * 1000.0f);
      shutterSetTravelTime(i, ms, config.jaro.ch_travel_time_up[i]);
    }
    if (strcmp(elementId, traveltimeUpId) == 0) {
      uint32_t ms = (uint32_t)(atof(value) * 1000.0f);
      shutterSetTravelTime(i, config.jaro.ch_travel_time[i], ms);
    }

    if (strcmp(elementId, unlearnId) == 0) {
      jaroCmd(CMD_UNLEARN, i);
      ESP_LOGI(TAG, "cmd UNLEARN - channel %i", i + 1);
      webUI.wsShowInfoMsg(WEB_TXT::CMD_UNLEARN[config.lang]);
    }
    if (strcmp(elementId, learnId) == 0) {
      jaroCmd(CMD_LEARN, i);
      ESP_LOGI(TAG, "cmd LEARN - channel %i", i + 1);
      webUI.wsShowInfoMsg(WEB_TXT::CMD_LEARN[config.lang]);
    }
    if (strcmp(elementId, cmdUpId) == 0) {
      ESP_LOGI(TAG, "cmd UP - channel %i", i + 1);
      jaroCmd(CMD_UP, i);
      webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_UP[config.lang]);
    }
    if (strcmp(elementId, cmdStopId) == 0) {
      ESP_LOGI(TAG, "cmd STOP - channel %i", i + 1);
      jaroCmd(CMD_STOP, i);
      webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_STOP[config.lang]);
    }
    if (strcmp(elementId, cmdDownId) == 0) {
      ESP_LOGI(TAG, "cmd DOWN - channel %i", i + 1);
      jaroCmd(CMD_DOWN, i);
      webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_DOWN[config.lang]);
    }
    if (strcmp(elementId, cmdShadeId) == 0) {
      ESP_LOGI(TAG, "cmd SHADE - channel %i", i + 1);
      jaroCmd(CMD_SHADE, i);
      webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_SHADE[config.lang]);
    }

    // Positionsschieberegler
    char cmdPosId[32];
    snprintf(cmdPosId, sizeof(cmdPosId), "p01_pos_%d", i);
    if (strcmp(elementId, cmdPosId) == 0) {
      int8_t targetPos = (int8_t)atoi(value);
      ESP_LOGI(TAG, "cmd SET_POSITION %d%% - channel %i", targetPos, i + 1);
      bool ok = shutterPosSetTarget(i, targetPos,
          [](JaroCmdType t, uint8_t ch){ jaroCmd(t, ch); },  // stopCb
          [](JaroCmdType t, uint8_t ch){ jaroCmd(t, ch); },  // upCb
          [](JaroCmdType t, uint8_t ch){ jaroCmd(t, ch); }); // downCb
      if (ok) {
        char msg[32];
        snprintf(msg, sizeof(msg), "Position %d%%", targetPos);
        webUI.wsShowInfoMsg(msg);
      } else {
        webUI.wsShowInfoMsg("Nicht kalibriert!");
      }
    }
  }

  // Kalibrierung DOWN: Start
  if (strcmp(elementId, "p04_calib_start") == 0) {
    ESP_LOGI(TAG, "calib DOWN START - channel %i", srvShutter + 1);
    shutterCalibDownStart(srvShutter);
    jaroCmd(CMD_DOWN, srvShutter);
    webUI.wsShowInfoMsg("Kalibrierung DOWN gestartet...");
  }

  // Kalibrierung DOWN: Finish → dann automatisch UP starten
  if (strcmp(elementId, "p04_calib_finish") == 0) {
    jaroCmd(CMD_STOP, srvShutter);
    uint32_t travelMs = shutterCalibDownFinish(srvShutter);
    ESP_LOGI(TAG, "calib DOWN FINISH - channel %i - %lu ms", srvShutter + 1, travelMs);
    // Immediately start UP calibration
    shutterCalibUpStart(srvShutter);
    jaroCmd(CMD_UP, srvShutter);
    char msg[48];
    snprintf(msg, sizeof(msg), "DOWN: %.1f s – jetzt UP...", travelMs / 1000.0f);
    webUI.wsShowInfoMsg(msg);
  }

  // Kalibrierung UP: Finish
  if (strcmp(elementId, "p04_calib_finish_up") == 0) {
    jaroCmd(CMD_STOP, srvShutter);
    uint32_t travelMs = shutterCalibUpFinish(srvShutter);
    ESP_LOGI(TAG, "calib UP FINISH - channel %i - %lu ms", srvShutter + 1, travelMs);
    configSaveToFile(); // beide Laufzeiten dauerhaft speichern
    char msg[64];
    snprintf(msg, sizeof(msg), "Kalibrierung OK! UP: %.1f s", travelMs / 1000.0f);
    webUI.wsShowInfoMsg(msg);
  }

  // group 1-6
  for (int i = 0; i < 6; i++) {
    char enableId[32];
    char nameId[32];
    char maskId[32];
    char cmdUpId[32];
    char cmdDownId[32];
    char cmdStopId[32];
    char cmdShadeId[32];

    snprintf(enableId, sizeof(enableId), "cfg_jaro_grp_enable_%d", i);
    snprintf(nameId, sizeof(nameId), "cfg_jaro_grp_name_%d", i);
    snprintf(maskId, sizeof(maskId), "cfg_jaro_grp_mask_%d", i);

    snprintf(cmdUpId, sizeof(cmdUpId), "p02_up_%d", i);
    snprintf(cmdDownId, sizeof(cmdDownId), "p02_down_%d", i);
    snprintf(cmdStopId, sizeof(cmdStopId), "p02_stop_%d", i);
    snprintf(cmdShadeId, sizeof(cmdShadeId), "p02_shade_%d", i);

    if (strcmp(elementId, enableId) == 0) {
      config.jaro.grp_enable[i] = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, nameId) == 0) {
      snprintf(config.jaro.grp_name[i], sizeof(config.jaro.grp_name[i]), "%s", value);
    }
    if (strcmp(elementId, maskId) == 0) {
      config.jaro.grp_mask[i] = strtoul(value, NULL, 2);
    }
    if (strcmp(elementId, cmdUpId) == 0) {
      ESP_LOGI(TAG, "cmd UP - group %i", i + 1);
      jaroCmd(CMD_GRP_UP, config.jaro.grp_mask[i]);
      webUI.wsShowInfoMsg(WEB_TXT::GROUP_CMD_UP[config.lang]);
    }
    if (strcmp(elementId, cmdStopId) == 0) {
      ESP_LOGI(TAG, "cmd STOP - group %i", i + 1);
      jaroCmd(CMD_GRP_STOP, config.jaro.grp_mask[i]);
      webUI.wsShowInfoMsg(WEB_TXT::GROUP_CMD_STOP[config.lang]);
    }
    if (strcmp(elementId, cmdDownId) == 0) {
      ESP_LOGI(TAG, "cmd DOWN - group %i", i + 1);
      jaroCmd(CMD_GRP_DOWN, config.jaro.grp_mask[i]);
      webUI.wsShowInfoMsg(WEB_TXT::GROUP_CMD_DOWN[config.lang]);
    }
    if (strcmp(elementId, cmdShadeId) == 0) {
      ESP_LOGI(TAG, "cmd SHADE - group %i", i + 1);
      jaroCmd(CMD_GRP_SHADE, config.jaro.grp_mask[i]);
      webUI.wsShowInfoMsg(WEB_TXT::GROUP_CMD_SHADE[config.lang]);
    }
  }

  // Channel timers (ch_timer_0 .. ch_timer_15)
  if (strncmp(elementId, "ch_timer_", 9) == 0) {
  for (int i = 0; i < 16; i++) {
    char prefix[32];
    char id[48];
    snprintf(prefix, sizeof(prefix), "ch_timer_%d", i);

    snprintf(id, sizeof(id), "%s_enable",    prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].enable    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_monday",    prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].monday    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_tuesday",   prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].tuesday   = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_wednesday", prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].wednesday = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_thursday",  prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].thursday  = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_friday",    prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].friday    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_saturday",  prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].saturday  = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_sunday",    prefix); if (strcmp(elementId, id) == 0) { config.ch_timer[i].sunday    = EspStrUtil::stringToBool(value); }

    char upPrefix[48], downPrefix[48];
    snprintf(upPrefix,   sizeof(upPrefix),   "%s_up",   prefix);
    snprintf(downPrefix, sizeof(downPrefix), "%s_down", prefix);
    processTimerEventField(elementId, value, upPrefix,   config.ch_timer[i].up);
    processTimerEventField(elementId, value, downPrefix, config.ch_timer[i].down);
  }
  } // end ch_timer_ check

  // Group timers (grp_timer_0 .. grp_timer_5)
  if (strncmp(elementId, "grp_timer_", 10) == 0) {
  for (int i = 0; i < 6; i++) {
    char prefix[32];
    char id[48];
    snprintf(prefix, sizeof(prefix), "grp_timer_%d", i);

    snprintf(id, sizeof(id), "%s_enable",    prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].enable    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_monday",    prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].monday    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_tuesday",   prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].tuesday   = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_wednesday", prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].wednesday = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_thursday",  prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].thursday  = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_friday",    prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].friday    = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_saturday",  prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].saturday  = EspStrUtil::stringToBool(value); }
    snprintf(id, sizeof(id), "%s_sunday",    prefix); if (strcmp(elementId, id) == 0) { config.grp_timer[i].sunday    = EspStrUtil::stringToBool(value); }

    char upPrefix[48], downPrefix[48];
    snprintf(upPrefix,   sizeof(upPrefix),   "%s_up",   prefix);
    snprintf(downPrefix, sizeof(downPrefix), "%s_down", prefix);
    processTimerEventField(elementId, value, upPrefix,   config.grp_timer[i].up);
    processTimerEventField(elementId, value, downPrefix, config.grp_timer[i].down);
  }
  } // end grp_timer_ check

  // Timer-Aenderungen speichern
  if (strncmp(elementId, "ch_timer_", 9) == 0 || strncmp(elementId, "grp_timer_", 10) == 0) {
    configSaveToFile();
  }

  // remote 1-16
  for (int i = 0; i < 16; i++) {
    char enableId[32];
    char nameId[32];
    char maskId[32];
    char serialId[32];

    snprintf(enableId, sizeof(enableId), "cfg_jaro_remote_enable_%d", i);
    snprintf(nameId, sizeof(nameId), "cfg_jaro_remote_name_%d", i);
    snprintf(maskId, sizeof(maskId), "cfg_jaro_remote_mask_%d", i);
    snprintf(serialId, sizeof(serialId), "cfg_jaro_remote_serial_%d", i);

    if (strcmp(elementId, enableId) == 0) {
      config.jaro.remote_enable[i] = EspStrUtil::stringToBool(value);
    }
    if (strcmp(elementId, nameId) == 0) {
      snprintf(config.jaro.remote_name[i], sizeof(config.jaro.remote_name[i]), "%s", value);
    }
    if (strcmp(elementId, maskId) == 0) {
      config.jaro.remote_mask[i] = strtoul(value, NULL, 2);
    }
    if (strcmp(elementId, serialId) == 0) {
      config.jaro.remote_serial[i] = strtoul(value, NULL, 16);
    }
  }

  // Shutter Service commands
  if (strcmp(elementId, "p04_selected_shutter") == 0) {
    srvShutter = atoi(value);
    ESP_LOGD(TAG, "selected service shutter: %i", srvShutter);
  }
  if (strcmp(elementId, "p04_up") == 0) {
    jaroCmd(CMD_UP, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_UP[config.lang]);
  }
  if (strcmp(elementId, "p04_stop") == 0) {
    jaroCmd(CMD_STOP, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_STOP[config.lang]);
  }
  if (strcmp(elementId, "p04_down") == 0) {
    jaroCmd(CMD_DOWN, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_DOWN[config.lang]);
  }
  if (strcmp(elementId, "p04_shade") == 0) {
    jaroCmd(CMD_SHADE, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::SHUTTER_CMD_SHADE[config.lang]);
  }
  if (strcmp(elementId, "p04_cmd_set_shade") == 0) {
    jaroCmd(CMD_SET_SHADE, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::CMD_SET_SHADE[config.lang]);
  }
  if (strcmp(elementId, "p04_cmd_end_up_set") == 0) {
    jaroCmd(CMD_SET_END_POINT_UP, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::CMD_SET_ENDPOINT_UP[config.lang]);
  }
  if (strcmp(elementId, "p04_cmd_end_down_set") == 0) {
    jaroCmd(CMD_SET_END_POINT_DOWN, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::CMD_SET_ENDPOINT_DOWN[config.lang]);
  }
  if (strcmp(elementId, "p04_cmd_end_up_delete") == 0) {
    jaroCmd(CMD_DEL_END_POINT_UP, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::CMD_DEL_ENDPOINT_UP[config.lang]);
  }
  if (strcmp(elementId, "p04_cmd_end_down_delete") == 0) {
    jaroCmd(CMD_DEL_END_POINT_DOWN, srvShutter);
    webUI.wsShowInfoMsg(WEB_TXT::CMD_DEL_ENDPOINT_DOWN[config.lang]);
  }

  // Language
  if (strcmp(elementId, "cfg_lang") == 0) {
    config.lang = strtoul(value, NULL, 10);
    updateAllElements();
  }

  // Restart (and save)
  if (strcmp(elementId, "restartAction") == 0) {
    EspSysUtil::RestartReason::saveLocal("webUI command");
    configSaveToFile();
    delay(1000);
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "cfg_logger_enable") == 0) {
    config.log.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_logger_level") == 0) {
    config.log.level = strtoul(value, NULL, 10);
    setLogLevel(config.log.level);
    clearLogBuffer();
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "cfg_logger_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    webUI.wsUpdateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }
}