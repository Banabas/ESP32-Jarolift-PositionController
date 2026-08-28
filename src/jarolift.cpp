#include <JaroliftController.h>
#include <shutter_position.h>
#include <basics.h>
#include <config.h>
#include <jarolift.h>
#include <mqtt.h>
#include <queue>
#include <timer.h>

#define MAX_CMD 20
#define SEND_CYCLE 500
// Position convention: 0% = closed, 100% = open (matches Home Assistant)
#define POS_OPEN 100
#define POS_CLOSE 0
#define POS_SHADE 10

static muTimer cmdTimer = muTimer();
static muTimer timerTimer = muTimer();
static const char *TAG = "JARO"; // LOG TAG

/** Channel label for log output: configured name, or "Ch N" fallback. */
static const char *chName(uint8_t ch) {
  static char buf[24];
  if (ch < 16 && config.jaro.ch_name[ch][0]) return config.jaro.ch_name[ch];
  snprintf(buf, sizeof(buf), "Ch %u", ch + 1u);
  return buf;
}

std::queue<JaroCommand> jaroCmdQueue;

JaroliftController jarolift;

/**
 * *******************************************************************
 * @brief   send expected position via mqtt
 * @param   channel, position
 * @return  none
 * *******************************************************************/
void mqttSendPosition(uint8_t channel, uint8_t position) {
  char topic[64];
  char pos[16];

  if (position > 100)
    position = 100;
  if (position < 0)
    position = 0;
  if (mqttIsConnected()) {
    itoa(position, pos, 10);
    snprintf(topic, sizeof(topic), "%s%d", addTopic("/status/shutter/"), channel + 1);
    mqttPublish(topic, pos, true);
  }
}

/**
 * *******************************************************************
 * @brief   send expected position via mqtt
 * @param   channel, position
 * @return  none
 * *******************************************************************/
void mqttSendPositionGroup(uint16_t group_mask, uint8_t position) {

  // check all 16 channels
  for (uint8_t c = 0; c < 16; c++) {
    // check if channel is in group
    if (group_mask & (1 << c)) {
      mqttSendPosition(c, position);
    }
  }
}

/**
 * *******************************************************************
 * @brief   send remote command via mqtt
 * @param   serial, function, rssi
 * @return  none
 * *******************************************************************/
void mqttSendRemote(uint32_t serial, int8_t function, uint16_t channel) {

  char fun[8];
  char chBIN[18];
  int pos = 0;
  for (int i = 15; i >= 0; i--) {
    chBIN[pos++] = ((channel >> i) & 1) ? '1' : '0';
    if (i == 8) {
      chBIN[pos++] = ' ';
    }
  }
  chBIN[pos] = '\0';

  switch (function) {
  case 0x2:
    snprintf(fun, sizeof(fun), "DOWN");
    break;
  case 0x3:
    snprintf(fun, sizeof(fun), "SHADE");
    break;
  case 0x4:
    snprintf(fun, sizeof(fun), "STOP");
    break;
  case 0x8:
    snprintf(fun, sizeof(fun), "UP");
    break;
  default:
    snprintf(fun, sizeof(fun), "0x%x", function);
    break;
  }

  // unknown as default
  const char *remoteName = "unknown";
  ESP_LOGD(TAG, "remote lookup: received serial=0x%06lx (>> 8)", serial >> 8);
  for (int i = 0; i < 16; i++) {
    if (!config.jaro.remote_enable[i]) continue;
    ESP_LOGD(TAG, "  slot %d: enabled, stored serial=0x%06lx, match=%s",
             i, config.jaro.remote_serial[i],
             (serial >> 8 == config.jaro.remote_serial[i]) ? "YES" : "no");
    if (serial >> 8 == config.jaro.remote_serial[i]) {
      remoteName = config.jaro.remote_name[i];

      // check if this remote is registered for one or more shutter
      for (int j = 0; j < 16; j++) {
        if (config.jaro.remote_mask[i] & (1 << j)) {
          // Feed remote-triggered moves into the same time-based position
          // tracker used for our own commands, so a shutter operated
          // manually via remote (e.g. stopped mid-travel, then driven the
          // other way) keeps an accurate estimated position.
          const bool tracked = config.jaro.ch_enable[j] && config.jaro.ch_travel_time[j] != 0;
          switch (function) {
          case 0x2: // DOWN
            if (tracked) {
              shutterPosNotifyDown(j);
              mqttSendPosition(j, shutterPosGet(j));
            } else {
              mqttSendPosition(j, POS_CLOSE);
            }
            break;
          case 0x8: // UP
            if (tracked) {
              shutterPosNotifyUp(j);
              mqttSendPosition(j, shutterPosGet(j));
            } else {
              mqttSendPosition(j, POS_OPEN);
            }
            break;
          case 0x4: // STOP
            if (tracked) {
              shutterPosNotifyStop(j);
              mqttSendPosition(j, shutterPosGet(j));
            }
            break;
          case 0x3: // SHADE
            mqttSendPosition(j, POS_SHADE);
            break;
          default:
            break;
          }
        }
      }
      break; // stop is first remote was found
    }
  }

  ESP_LOGI(TAG, "received remote signal | name: %s | serial: 0x%08lx | cmd: %s | channel: %s", remoteName, serial, fun, chBIN);

  if (mqttIsConnected()) {
    JsonDocument remoteJSON;
    remoteJSON["name"] = remoteName;
    remoteJSON["cmd"] = fun;
    remoteJSON["chBin"] = chBIN;
    remoteJSON["chDec"] = channel;

    char sendremoteJSON[255];
    serializeJson(remoteJSON, sendremoteJSON);

    char topic[64];
    snprintf(topic, sizeof(topic), "%s%08lx", addTopic("/status/remote/"), serial);

    mqttPublish(topic, sendremoteJSON, false);
  }
}

/**
 * *******************************************************************
 * @brief   add jarolift command to buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void jaroCmd(JaroCmdType type, uint8_t channel) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::SINGLE, {.single = {type, channel}}});
    ESP_LOGD(TAG, "add single cmd to buffer: %i, %i", type, channel + 1);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

void jaroCmd(JaroCmdGrpType type, uint16_t group_mask) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::GROUP, {.group = {type, group_mask}}});
    ESP_LOGD(TAG, "add group cmd to buffer: %i, %04X", type, group_mask);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

void jaroCmd(JaroCmdSrvType type, uint8_t channel) {
  if (jaroCmdQueue.size() < MAX_CMD) {
    jaroCmdQueue.push({JaroCommand::SERVICE, {.service = {type, channel}}});
    ESP_LOGD(TAG, "add service cmd to buffer: %i, %i", type, channel + 1);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

/**
 * *******************************************************************
 * @brief   Jarolift Setup function
 * @param   none
 * @return  none
 * *******************************************************************/
// Send CMD_STOP immediately, bypassing the 500ms command queue.
// Use only for time-critical position stops.
void jaroStopNow(uint8_t channel) {
  // Send stop RF telegram immediately, bypassing the 500ms queue.
  // Do NOT call shutterPosNotifyStop here – the caller (shutterPosUpdate)
  // updates the state itself immediately after this call.
  jarolift.cmdChannel(JaroliftController::CMD_STOP, channel);
  ESP_LOGI(TAG, "immediate STOP - channel: %s", chName(channel));
}

void jaroliftSetup() {

  // initialize
  ESP_LOGI(TAG, "initializing the CC1101 Transceiver");
  jarolift.setGPIO(config.gpio.sck, config.gpio.miso, config.gpio.mosi, config.gpio.cs, config.gpio.gdo0, config.gpio.gdo2);
  jarolift.setKeys(config.jaro.masterMSB, config.jaro.masterLSB);
  jarolift.setBaseSerial(config.jaro.serial);
  jarolift.setLegacyLearnMode(!config.jaro.learn_mode);
  jarolift.begin();

  if (jarolift.getCC1101State()) {
    ESP_LOGI(TAG, "CC1101 Transceiver connected!");
  } else {
    ESP_LOGE(TAG, "CC1101 Transceiver NOT connected!");
  }

  ESP_LOGI(TAG, "read Device Counter from FLASH: %i", jarolift.getDeviceCounter());

  jarolift.setRemoteCallback(mqttSendRemote);
  shutterPosInit();
}

void jaroCmdReInit() { jaroliftSetup(); };
void jaroCmdSetDevCnt(uint16_t value) { jarolift.setDeviceCounter(value); };
uint16_t jaroGetDevCnt() { return jarolift.getDeviceCounter(); };
bool getCC1101State() { return jarolift.getCC1101State(); };
uint8_t getCC1101Rssi() { return jarolift.getRssi(); }

/**
 * *******************************************************************
 * @brief   execute jarolift commands from buffer
 * @param   type, channel
 * @return  none
 * *******************************************************************/
void processJaroCommands() {
  if (!jaroCmdQueue.empty()) {
    JaroCommand cmd = jaroCmdQueue.front();

    if (cmd.cmdType == JaroCommand::SINGLE) {
      switch (cmd.single.type) {
      case CMD_UP:
        jarolift.cmdChannel(JaroliftController::CMD_UP, cmd.single.channel);
        shutterPosNotifyUp(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_OPEN);
        ESP_LOGI(TAG, "execute cmd: UP - channel: %s", chName(cmd.single.channel));
        break;
      case CMD_DOWN:
        jarolift.cmdChannel(JaroliftController::CMD_DOWN, cmd.single.channel);
        shutterPosNotifyDown(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_CLOSE);
        ESP_LOGI(TAG, "execute cmd: DOWN - channel: %s", chName(cmd.single.channel));
        break;
      case CMD_STOP:
        jarolift.cmdChannel(JaroliftController::CMD_STOP, cmd.single.channel);
        shutterPosNotifyStop(cmd.single.channel);
        mqttSendPosition(cmd.single.channel, shutterPosGet(cmd.single.channel));
        ESP_LOGI(TAG, "execute cmd: STOP - channel: %s", chName(cmd.single.channel));
        break;
      case CMD_SET_SHADE:
        jarolift.cmdChannel(JaroliftController::CMD_SET_SHADE, cmd.single.channel);
        mqttSendPosition(cmd.single.channel, POS_SHADE);
        ESP_LOGI(TAG, "execute cmd: SETSHADE - channel: %s", chName(cmd.single.channel));
        break;
      case CMD_SHADE:
        jarolift.cmdChannel(JaroliftController::CMD_SHADE, cmd.single.channel);
        ESP_LOGI(TAG, "execute cmd: SHADE - channel: %s", chName(cmd.single.channel));
        break;
      }
    } else if (cmd.cmdType == JaroCommand::GROUP) {
      switch (cmd.group.type) {
      case CMD_GRP_UP:
        jarolift.cmdGroup(JaroliftController::CMD_UP, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: UP - mask: %04X", cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_OPEN);
        break;
      case CMD_GRP_DOWN:
        jarolift.cmdGroup(JaroliftController::CMD_DOWN, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: DOWN - mask: %04X", cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_CLOSE);
        break;
      case CMD_GRP_STOP:
        jarolift.cmdGroup(JaroliftController::CMD_STOP, cmd.group.group_mask);
        ESP_LOGI(TAG, "execute group cmd: STOP - mask: %04X", cmd.group.group_mask);
        break;
      case CMD_GRP_SHADE:
        jarolift.cmdGroup(JaroliftController::CMD_SHADE, cmd.group.group_mask);
        mqttSendPositionGroup(cmd.group.group_mask, POS_SHADE);
        ESP_LOGI(TAG, "execute group cmd: SHADE - mask: %04X", cmd.group.group_mask);
        break;
      }
    } else if (cmd.cmdType == JaroCommand::SERVICE) {
      switch (cmd.service.type) {
      case CMD_LEARN:
        jarolift.cmdLearn(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_LEARN - channel: %s", chName(cmd.service.channel));
        break;
      case CMD_UNLEARN:
        jarolift.cmdUnlearn(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_UNLEARN - channel: %s", chName(cmd.service.channel));
        break;
      case CMD_SET_END_POINT_UP:
        jarolift.cmdSetEndPointUp(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: SET_END_POINT_UP - channel: %s", chName(cmd.service.channel));
        break;
      case CMD_DEL_END_POINT_UP:
        jarolift.cmdDeleteEndPointUp(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_DEL_END_POINT_UP - channel: %s", chName(cmd.service.channel));
        break;
      case CMD_SET_END_POINT_DOWN:
        jarolift.cmdSetEndPointDown(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: SET_END_POINT_DOWN - channel: %s", chName(cmd.service.channel));
        break;
      case CMD_DEL_END_POINT_DOWN:
        jarolift.cmdDeleteEndPointDown(cmd.service.channel);
        ESP_LOGI(TAG, "execute service cmd: CMD_DEL_END_POINT_DOWN - channel: %s", chName(cmd.service.channel));
        break;
      }
    }

    jaroCmdQueue.pop();
  }
}

/**
 * *******************************************************************
 * @brief   jarolift cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void jaroliftCyclic() {

  if (cmdTimer.cycleTrigger(SEND_CYCLE)) {
    processJaroCommands();
  }

  // Position tracking: Zielposition prüfen und ggf. stoppen
  static uint32_t lastPosCheck = 0;
  if (millis() - lastPosCheck >= 100) {
    lastPosCheck = millis();
    shutterPosUpdate([](JaroCmdType t, uint8_t ch){ jaroCmd(t, ch); });
    // Only persist position when stopped to avoid triggering config saves every 100ms
    for (int i = 0; i < 16; i++) {
      if (config.jaro.ch_enable[i] && !shutterIsMoving(i))
        config.jaro.ch_last_pos[i] = shutterPosGet(i);
    }
  }

  if (timerTimer.cycleTrigger(10000)) {
    timerCyclic();
  }

  jarolift.loop();
}