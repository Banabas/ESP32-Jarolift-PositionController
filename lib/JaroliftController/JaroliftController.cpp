// JaroliftController.cpp
#include "JaroliftController.h"
#include "esp32-hal-periman.h"

// Definition of the static instance pointer
JaroliftController *JaroliftController::instance_ = nullptr;

JaroliftController::JaroliftController()
    : devCount_(0), deviceKeyMSB_(0), deviceKeyLSB_(0), button_(0), discL_(0), discH_(0), disc_(0), newSerial_(0), encrypted_(0), pack_(0),
      pbWrite_(0), frameReady_(false), rxSerial_(0), rxHopCode_(0), rxFunction_(0), rxDiscH_(0), initOK_(false), steadyCount_(0), rxDataReady_(false), txSymbolCount_(0), txHalfUsed_(false), rmtOK_(false), txStartMs_(0) {

  memset((void *)lowBuf_, 0, sizeof(lowBuf_));
  memset((void *)hiBuf_, 0, sizeof(hiBuf_));

  uint8_t defaultDiscLow[16] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  uint8_t defaultDiscHigh[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  memcpy(discLowArr_, defaultDiscLow, sizeof(discLowArr_));
  memcpy(discHighArr_, defaultDiscHigh, sizeof(discHighArr_));

  config_.serial = 0;
  config_.learnMode = true;
  config_.masterMSB = 0;
  config_.masterLSB = 0;

  // Set the Singleton instance
  instance_ = this;
}

JaroliftController::~JaroliftController() { instance_ = nullptr; }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// helper and setter functions
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *******************************************************************
 * @brief   set gpio for CC1101
 * @param   sck, miso, mosi, cs, gd0, gd2
 * @return  none
 * *******************************************************************/
void JaroliftController::setGPIO(int sck, int miso, int mosi, int cs, int gdo0, int gdo2) {
  gpio_.sck = sck;
  gpio_.miso = miso;
  gpio_.mosi = mosi;
  gpio_.cs = cs;
  gpio_.gdo0 = gdo0;
  gpio_.gdo2 = gdo2;
}

/**
 *******************************************************************
 * @brief   set serial number (6 of 8 bytes)
 * @param   serial
 * @return  none
 * *******************************************************************/
void JaroliftController::setBaseSerial(uint32_t serial) {
  config_.serial = serial;
  ESP_LOGI(TAG, "Set base serial: 0x%08lx", config_.serial);
}

/**
 *******************************************************************
 * @brief   get serial for given channel
 * @param   channel
 * @return  none
 * *******************************************************************/
uint32_t JaroliftController::getSerial(uint8_t channel) {
  uint32_t serial = (config_.serial << 8) | channel;
  ESP_LOGD(TAG, "serial: 0x%08lx | channel: %d", serial, channel + 1);
  return serial;
}

/**
 *******************************************************************
 * @brief   set legacy learn mode
 * @param   legacyMode
 * @return  none
 * *******************************************************************/
void JaroliftController::setLegacyLearnMode(bool legacyMode) { config_.learnMode = !legacyMode; }

/**
 *******************************************************************
 * @brief   set jarolift master keys
 * @param   masterMSB, masterLSB
 * @return  none
 * *******************************************************************/
void JaroliftController::setKeys(unsigned long masterMSB, unsigned long masterLSB) {
  config_.masterMSB = masterMSB;
  config_.masterLSB = masterLSB;
}

/**
 *******************************************************************
 * @brief   get state if CC1101 is connected
 * @param   none
 * @return  none
 * *******************************************************************/
bool JaroliftController::getCC1101State() { return cc1101_.connected(); }

/**
 *******************************************************************
 * @brief   get state if CC1101 is connected
 * @param   none
 * @return  none
 * *******************************************************************/
uint16_t JaroliftController::getDeviceCounter() {
  nvs_handle_t nvsHandle;
  esp_err_t err = nvs_open("device_data", NVS_READWRITE, &nvsHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return 0;
  }
  uint16_t counter = 0;
  err = nvs_get_u16(nvsHandle, "devcnt", &counter);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    // try to read from EEPROM if not found in NVS (migration)
    EEPROM.get(0, counter);
    nvs_set_u16(nvsHandle, "devcnt", counter);
    nvs_commit(nvsHandle);
    ESP_LOGI(TAG, "Migrated devcnt=%d from EEPROM to NVS", counter);
  }
  nvs_close(nvsHandle);
  if (counter == 0) {
    counter = 1;
    setDeviceCounter(counter);
  }
  return counter;
}

/**
 *******************************************************************
 * @brief   set device counter
 * @param   newDevCnt
 * @return  none
 * *******************************************************************/
void JaroliftController::setDeviceCounter(uint16_t newDevCnt) {
  devCount_ = newDevCnt;
  nvs_handle_t nvsHandle;
  esp_err_t err = nvs_open("device_data", NVS_READWRITE, &nvsHandle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return;
  }
  nvs_set_u16(nvsHandle, "devcnt", devCount_);
  nvs_commit(nvsHandle);
  nvs_close(nvsHandle);
  delay(100);
}

/**
 *******************************************************************
 * @brief   update and increment device counter
 * @param   increment
 * @return  none
 * *******************************************************************/
void JaroliftController::updateDeviceCounter(bool increment) {
  devCount_ = getDeviceCounter();
  if (increment) {
    devCount_++;
    setDeviceCounter(devCount_);
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CC1101 radio functions group
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *******************************************************************
 * @brief   attach the RMT TX channel to GDO0
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::initRmt() {
  rmtOK_ = rmtInit(gpio_.gdo0, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_2, kRmtResolutionHz);
  if (rmtOK_) {
    rmtSetEOT(gpio_.gdo0, HIGH); // idle level after each telegram = carrier off
    rmt_data_t idle = {1, HIGH, 1, HIGH};
    rmtWrite(gpio_.gdo0, &idle, 1, RMT_WAIT_FOR_EVER); // set line to idle level
    ESP_LOGI(TAG, "RMT TX ready on GDO0 (GPIO %d)", gpio_.gdo0);
  } else {
    ESP_LOGE(TAG, "RMT init on GDO0 (GPIO %d) failed", gpio_.gdo0);
  }
}

/**
 *******************************************************************
 * @brief   append one level segment to the RMT symbol buffer
 * @details the CC1101 is in asynchronous TX mode: GDO0 LOW = carrier on
 *          (PATABLE[0]), GDO0 HIGH = carrier off (PATABLE[1])
 * @param   level, durationUs
 * @return  none
 * *******************************************************************/
void JaroliftController::txAddLevel(uint8_t level, uint16_t durationUs) {
  if (txSymbolCount_ >= kMaxTxSymbols) {
    ESP_LOGE(TAG, "RMT symbol buffer overflow");
    return;
  }
  rmt_data_t &sym = txSymbols_[txSymbolCount_];
  if (!txHalfUsed_) {
    sym.level0 = level;
    sym.duration0 = durationUs;
    sym.level1 = level; // placeholder, completed by the next segment
    sym.duration1 = 0;
    txHalfUsed_ = true;
  } else {
    sym.level1 = level;
    sym.duration1 = durationUs;
    txHalfUsed_ = false;
    txSymbolCount_++;
  }
}

/**
 *******************************************************************
 * @brief   Generates sync-pulses (protocoll start)
 * @param   length
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTxFrame(int length) {
  for (int i = 0; i < length; ++i) {
    txAddLevel(LOW, 400);
    txAddLevel(HIGH, 380);
  }
}

/**
 *******************************************************************
 * @brief   Sending of high_group_bits 8-16 (discH_)
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTxGroupH() {
  for (int i = 0; i < 8; i++) {
    int bitVal = (discH_ >> i) & 0x1;
    if (bitVal == 1) {
      txAddLevel(LOW, kLowPulse);
      txAddLevel(HIGH, kHighPulse);
    } else {
      txAddLevel(LOW, kHighPulse);
      txAddLevel(HIGH, kLowPulse);
    }
  }
}

/**
 *******************************************************************
 * @brief   tx routine to send data
 * @details The complete telegram is built as RMT symbols and output by
 *          the RMT peripheral. The pulse timing is therefore exact even
 *          on single core chips (ESP32-S2) where WiFi interrupts used to
 *          stretch the software delays and corrupt the telegram.
 * @param   repetitions
 * @return  none
 * *******************************************************************/
void JaroliftController::radioTx(int repetitions) {
  pack_ = (button_ << 60) | (newSerial_ << 32) | encrypted_;
  txStartMs_ = 0;
  if (perimanGetPinBusType(gpio_.gdo0) != ESP32_BUS_TYPE_RMT_TX) {
    ESP_LOGW(TAG, "GDO0 (GPIO %d) is no longer attached to RMT (bus type %d) - re-init", gpio_.gdo0, (int)perimanGetPinBusType(gpio_.gdo0));
    initRmt();
  }
  if (!rmtOK_) {
    ESP_LOGE(TAG, "RMT not initialized - telegram not sent");
    return;
  }

  // build telegram once
  txSymbolCount_ = 0;
  txHalfUsed_ = false;
  txAddLevel(LOW, 1150);
  radioTxFrame(13);
  txAddLevel(HIGH, 3500);
  for (int i = 0; i < 64; i++) {
    int bitVal = (pack_ >> i) & 0x1;
    if (bitVal == 1) {
      txAddLevel(LOW, kLowPulse);
      txAddLevel(HIGH, kHighPulse);
    } else {
      txAddLevel(LOW, kHighPulse);
      txAddLevel(HIGH, kLowPulse);
    }
  }
  radioTxGroupH();
  if (txHalfUsed_) { // odd number of segments: close last symbol with idle level
    txAddLevel(HIGH, 1);
  }

  int okCount = 0;
  for (int a = 0; a < repetitions; a++) {
    if (rmtWrite(gpio_.gdo0, txSymbols_, txSymbolCount_, RMT_WAIT_FOR_EVER)) {
      if (okCount == 0) {
        txStartMs_ = millis(); // the receiver acts on the first complete telegram
      }
      okCount++;
    } else {
      ESP_LOGE(TAG, "rmtWrite failed");
    }
    delay(16); // pause between repetitions, line stays HIGH (carrier off)
  }
  ESP_LOGI(TAG, "TX via RMT: %u symbols, %d/%d repetitions sent, MARCSTATE 0x%02X", (unsigned)txSymbolCount_, okCount, repetitions,
           cc1101_.readStatusReg(CC1101_MARCSTATE) & 0x1F);
}

/**
 *******************************************************************
 * @brief   put CC1101 to receive mode
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::enterRx() {
  cc1101_.setRxState();
  delay(2);
  unsigned long startTime = micros();
  uint8_t marcState = 0;
  while (((marcState = cc1101_.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x0D) {
    if (micros() - startTime > 50000)
      break;
  }
}

/**
 *******************************************************************
 * @brief   put CC1101 to send mode
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::enterTx() {
  cc1101_.setTxState();
  delay(2);
  unsigned long startTime = micros();
  uint8_t marcState = 0;
  while (((marcState = cc1101_.readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x13 && ((marcState & 0x1F) != 0x14) && ((marcState & 0x1F) != 0x15)) {
    if (micros() - startTime > 50000)
      break;
  }
  marcState &= 0x1F;
  if (marcState < 0x13 || marcState > 0x15) {
    // CC1101 did not enter TX - re-initialize it and try once more
    char reason[40];
    snprintf(reason, sizeof(reason), "no TX state (MARCSTATE 0x%02X)", marcState);
    if (reinitRadio(reason)) {
      cc1101_.setTxState();
      delay(2);
    }
  }
}

/**
 *******************************************************************
 * @brief   re-initialize the CC1101 (reset strobe + all registers) and
 *          the RMT output, e.g. after the chip stopped responding
 * @param   reason (for the log)
 * @return  true if the CC1101 responds again
 * *******************************************************************/
bool JaroliftController::reinitRadio(const char *reason) {
  ESP_LOGW(TAG, "CC1101 re-init: %s", reason);
  if (!cc1101_.init()) {
    ESP_LOGE(TAG, "CC1101 re-init failed - chip not responding");
    return false;
  }
  cc1101_.setSyncWord(kSyncWord, false);
  cc1101_.setCarrierFreq(CFREQ_433);
  cc1101_.disableAddressCheck();
  initRmt(); // init() sets GDO0 to input - attach RMT again
  return true;
}

/**
 *******************************************************************
 * @brief   read a CC1101 register (0x00-0x2E config, 0x30-0x3D status)
 * *******************************************************************/
uint8_t JaroliftController::ccReadReg(uint8_t reg) {
  return (reg >= 0x30) ? cc1101_.readReg(reg, CC1101_STATUS_REGISTER) : cc1101_.readReg(reg, CC1101_CONFIG_REGISTER);
}

/**
 *******************************************************************
 * @brief   write a CC1101 config register (in IDLE, then back to RX)
 * *******************************************************************/
void JaroliftController::ccWriteReg(uint8_t reg, uint8_t value) {
  if (reg > 0x2E)
    return;
  cc1101_.cmdStrobe(CC1101_SIDLE);
  delay(2);
  cc1101_.writeReg(reg, value);
  enterRx();
}

/**
 *******************************************************************
 * @brief   calculate RSSI value (Received Signal Strength Indicator)
 * @param   none
 * @return  none
 * *******************************************************************/
uint8_t JaroliftController::getRssi() {
  uint8_t rssi = cc1101_.readReg(CC1101_RSSI, CC1101_STATUS_REGISTER);
  uint8_t value = 0;
  if (rssi >= 128) {
    value = 255 - rssi;
    value = value / 2;
    value = value + 74;
  } else {
    value = rssi / 2;
    value = value + 74;
  }
  return value;
}

/**
 *******************************************************************
 * @brief   Calculate device keys based on serial and master keys
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::generateKey() {
  Keeloq k(config_.masterMSB, config_.masterLSB);
  uint64_t keyInput = newSerial_ | 0x20000000;
  unsigned long enc = k.decrypt(keyInput);
  deviceKeyLSB_ = enc;
  keyInput = newSerial_ | 0x60000000;
  enc = k.decrypt(keyInput);
  deviceKeyMSB_ = enc;
}

/**
 *******************************************************************
 * @brief   Generation of the encrypted message (Hopcode)
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::generateEncrypted() {
  Keeloq k(deviceKeyMSB_, deviceKeyLSB_);
  devCount_ = getDeviceCounter();
  unsigned int result = (disc_ << 16) | devCount_; // Append counter value to discrimination value
  encrypted_ = k.encrypt(result);
}

/**
 *******************************************************************
 * @brief   encrypt device keys from received hopcode based on received Serialnumber
 * @details Here normal key-generation is used according to 00745a_c.PDF Appendix G.
 * @details https://github.com/hnhkj/documents/blob/master/KEELOQ/docs/AN745/00745a_c.pdf
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::rxKeyGen() {
  Keeloq k(config_.masterMSB, config_.masterLSB);
  uint32_t keylow = rxSerial_ | 0x20000000;
  unsigned long enc = k.decrypt(keylow);
  deviceKeyLSB_ = enc; // Stores LSB devicekey 16Bit
  keylow = rxSerial_ | 0x60000000;
  enc = k.decrypt(keylow);
  deviceKeyMSB_ = enc; // Stores MSB devicekey 16Bit
}

/**
 *******************************************************************
 * @brief   encrypt received hopcode based calculated device keys
 * @param   none
 * @return  none
 * *******************************************************************/
uint32_t JaroliftController::rxDecode() {
  Keeloq k(deviceKeyMSB_, deviceKeyLSB_);
  unsigned int result = rxHopCode_;
  return k.decrypt(result);
}

/**
 *******************************************************************
 * @brief   Interrupt-Service-Routine (ISR)
 * @param   none
 * @return  none
 * *******************************************************************/
void IRAM_ATTR JaroliftController::radioRxMeasureISR() {
  if (instance_) {
    instance_->handleRadioRxMeasure();
  }
}

/**
 *******************************************************************
 * @brief   Handling incoming data
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::handleRadioRxMeasure() {
  static unsigned long lineUp = 0;
  static unsigned long lineDown = 0;
  static unsigned long timeout = 0;
  static bool framePending = false; // all bits received, confirmed by the next valid falling edge
  unsigned long currentMicros = micros();
  int pinState = digitalRead(gpio_.gdo2);
  rxEdgeCount_++;
  // keep a received telegram until processRxData() has read it - the remote
  // sends one telegram per key press, it must not be overwritten while the
  // main loop is busy (e.g. saving the config)
  if (frameReady_ || (currentMicros - timeout > 3500 && pbWrite_ >= kMinTelegram && isSync(lowBuf_[0]))) {
    frameReady_ = true;   // pause after a shorter telegram
    framePending = false; // frame completed by the pause, not by the next falling edge
    if (pinState)
      lineUp = currentMicros;
    else
      lineDown = currentMicros;
    return;
  }
  if (currentMicros - timeout > 3500) {
    pbWrite_ = 0;
  }
  // Guard against buffer overrun: continuous RF noise (pulses 300..1000us
  // without a >3.5ms gap) never resets pbWrite_, which would otherwise write
  // past lowBuf_/hiBuf_ and corrupt pbWrite_ itself and adjacent globals
  // (e.g. the command queue). A valid telegram has max. ~75 pulses.
  if (pbWrite_ >= kPulseBufferSize) {
    pbWrite_ = 0;
  }
  // Glitch filter: a pulse shorter than kDebounce is a noise spike inside a
  // longer pulse. Both edges of the spike are ignored and the surrounding
  // pulse is measured in full - previously the spike cut the pulse in two
  // and the telegram failed the plausibility check or was read bit-shifted.
  static unsigned long prevLineUp = 0;
  static unsigned long prevLineDown = 0;
  static unsigned long prevTimeout = 0;
  static unsigned int prevPbWrite = 0;
  static unsigned int prevLowBuf0 = 0;
  static bool lastRiseStored = false;
  static bool lastRiseSync = false; // last rising edge stored a sync gap in lowBuf_[0]

  if (pinState) { // Übergang zu HIGH
    unsigned long lowVal = currentMicros - lineDown;
    if (lowVal < kDebounce) {
      // short LOW spike inside a HIGH pulse: undo its falling edge
      lineDown = prevLineDown;
      lastRiseStored = false;
      return;
    }
    prevLineUp = lineUp;
    prevPbWrite = pbWrite_;
    prevTimeout = timeout;
    lineUp = currentMicros;
    lastRiseStored = false;
    // short pulses down to 200 us are accepted: the TDRC16 sends 290-420 us,
    // the demodulator can shorten them further
    lastRiseSync = false;
    if (lowVal > 3650 && lowVal < 4300) {
      timeout = currentMicros;
      prevLowBuf0 = lowBuf_[0]; // restored if this edge turns out to be a spike
      pbWrite_ = 0;
      lowBuf_[pbWrite_] = lowVal;
      pbWrite_++;
      lastRiseStored = true;
      lastRiseSync = true;
    } else if (lowVal > 200 && lowVal < 1000) {
      lowBuf_[pbWrite_] = lowVal;
      pbWrite_++;
      timeout = currentMicros;
      lastRiseStored = true;
      if (pbWrite_ >= kFullTelegram && isSync(lowBuf_[0])) {
        // all 72 bits received - frameReady_ is set by the next falling edge,
        // so that a spike at this edge can still be undone
        framePending = true;
      }
    }
  } else { // Übergang zu LOW
    unsigned long highVal = currentMicros - lineUp;
    if (highVal < kDebounce) {
      // short HIGH spike inside a LOW pulse: undo its rising edge
      lineUp = prevLineUp;
      if (lastRiseStored) {
        pbWrite_ = prevPbWrite;
        timeout = prevTimeout;
        if (lastRiseSync)
          lowBuf_[0] = prevLowBuf0;
      }
      lastRiseStored = false;
      lastRiseSync = false;
      framePending = false;
      return;
    }
    prevLineDown = lineDown;
    lineDown = currentMicros;
    lastRiseStored = false;
    lastRiseSync = false;
    // always write: an out-of-range pulse is stored as 0 (invalid), so that a
    // partial value from an earlier spike inside this pulse does not remain
    hiBuf_[pbWrite_] = (highVal > 200 && highVal < 1000) ? highVal : 0;
    if (framePending) {
      framePending = false;
      frameReady_ = true; // all 72 bits received
    }
  }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Shutter command functions
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 * *******************************************************************
 * @brief   send channel commands (Up, DOWN, STOP, SHADE)
 * @param   cmd, channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdChannel(commands cmd, uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);

  switch (cmd) {
  case CMD_UP:
    button_ = FCT_CODE_UP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(kTxRepeat); // send command kTxRepeat-times with same devCnt
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_DOWN:
    button_ = FCT_CODE_DOWN;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(kTxRepeat); // send command kTxRepeat-times with same devCnt
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_STOP:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(kTxRepeatStop); // STOP only 2-times: repeated STOP = "continuous STOP" (shade)
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_SHADE:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    generateEncrypted();
    enterTx();
    radioTx(20); // send "continuos STOP"
    enterRx();
    updateDeviceCounter(true);
    break;

  case CMD_SET_SHADE:
    button_ = FCT_CODE_STOP;
    discL_ = discLowArr_[channel];
    discH_ = discHighArr_[channel];
    disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
    generateKey();
    // send 4-times STOP
    for (int i = 0; i < 4; i++) {
      enterTx();
      generateEncrypted();
      radioTx(1);
      updateDeviceCounter(true);
      enterRx();
      delay(300);
    }
    break;

  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   send group commands (Up, DOWN, STOP, SHADE)
 * @param   cmd, channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdGroup(commands cmd, uint16_t groupMask) {
  if (!initOK_)
    return;
  devCount_ = getDeviceCounter();
  newSerial_ = getSerial(0);

  switch (cmd) {
  case CMD_UP:
    button_ = FCT_CODE_UP;
    break;

  case CMD_DOWN:
    button_ = FCT_CODE_DOWN;
    break;

  case CMD_STOP:
  case CMD_SHADE:
    button_ = FCT_CODE_STOP;
    break;

  default:
    break;
  }

  discL_ = groupMask & 0x00FF;
  discH_ = (groupMask >> 8) & 0x00FF;
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  if (cmd == CMD_SHADE) {
    radioTx(20); // send "continuos STOP"
  } else if (cmd == CMD_STOP) {
    radioTx(kTxRepeatStop); // STOP only 2-times: repeated STOP = "continuous STOP" (shade)
  } else {
    radioTx(kTxRepeat); // send command kTxRepeat-times with same devCnt
  }

  enterRx();
  updateDeviceCounter(true);
}

/**
 * *******************************************************************
 * @brief   send command to learn a new shutter
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdLearn(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "learn | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  button_ = config_.learnMode ? 0xA : 0x1;
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  radioTx(2);
  enterRx();
  updateDeviceCounter(true);
  if (config_.learnMode) {
    delay(1000);
    button_ = 0x4; // Stop
    generateEncrypted();
    enterTx();
    radioTx(2);
    enterRx();
    updateDeviceCounter(true);
  }
}

/**
 * *******************************************************************
 * @brief   send command to unlearn a existing shutter
 * @details UP/DOWN -> 6x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdUnlearn(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "unlearn | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(2);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 6; i++) {
    button_ = FCT_CODE_STOP; // 6x Stop
    enterTx();
    generateEncrypted();
    radioTx(2);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(2);
  updateDeviceCounter(true);
  enterRx();
  updateDeviceCounter(false);
}

/**
 * *******************************************************************
 * @brief   send command to set the upper end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdSetEndPointUp(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "set upper end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 2; i++) {
    button_ = FCT_CODE_STOP; // 2x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to delete the upper end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdDeleteEndPointUp(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "delete upper end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 4; i++) {
    button_ = FCT_CODE_STOP; // 4x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_UP; // UP
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to set the lower end point
 * @details UP/DOWN -> 2x STOP -> DOWN
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdSetEndPointDown(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "set lower end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 2; i++) {
    button_ = FCT_CODE_STOP; // 2x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_DOWN; // DOWN
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   send command to delete the lower end point
 * @details UP/DOWN -> 2x STOP -> UP
 * @param   channel
 * @return  none
 * *******************************************************************/
void JaroliftController::cmdDeleteEndPointDown(uint8_t channel) {
  if (!initOK_)
    return;
  newSerial_ = getSerial(channel);
  devCount_ = getDeviceCounter();
  ESP_LOGD(TAG, "delete lower end point | Device Counter: %d | Serial: 0x%08llx", devCount_, newSerial_);
  discL_ = discLowArr_[channel];
  discH_ = discHighArr_[channel];
  disc_ = (discL_ << 8) | (newSerial_ & 0xFF);
  generateKey();
  generateEncrypted();
  enterTx();
  button_ = FCT_CODE_UPDOWN; // Up+Down
  radioTx(1);
  enterRx();
  updateDeviceCounter(true);
  delay(300);
  for (int i = 0; i < 4; i++) {
    button_ = FCT_CODE_STOP; // 4x Stop
    enterTx();
    generateEncrypted();
    radioTx(1);
    updateDeviceCounter(true);
    enterRx();
    delay(300);
  }
  button_ = FCT_CODE_DOWN; // DOWN
  enterTx();
  generateEncrypted();
  radioTx(1);
  updateDeviceCounter(true);
  enterRx();
}

/**
 * *******************************************************************
 * @brief   process received data
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::processRxData() {

  // decode only a complete telegram, marked by the RX interrupt: all 72 bits
  // received, or a shorter telegram followed by a pause. Decoding at pulse 65
  // read the group bits 9-16 before they were received - the channel then did
  // not match.
  if (!frameReady_)
    return;
  const unsigned int len = pbWrite_;
  if (isSync(lowBuf_[0]) && len >= kMinTelegram && len <= 75) {
    rxDataReady_ = true;
    pbWrite_ = 0;

    // extract Hopcode (32 Bit)
    rxHopCode_ = 0;
    for (int i = 0; i < 32; i++) {
      if (lowBuf_[i + 1] < hiBuf_[i + 1])
        rxHopCode_ &= ~(1 << i);
      else
        rxHopCode_ |= (1 << i);
    }

    // extract Serial (28 Bit)
    rxSerial_ = 0;
    for (int i = 0; i < 28; i++) {
      if (lowBuf_[i + 33] < hiBuf_[i + 33])
        rxSerial_ &= ~(1 << i);
      else
        rxSerial_ |= (1 << i);
    }

    // extract function code (4 Bit)
    rxFunction_ = 0;
    for (int i = 0; i < 4; i++) {
      if (lowBuf_[61 + i] < hiBuf_[61 + i])
        rxFunction_ &= ~(1 << i);
      else
        rxFunction_ |= (1 << i);
    }
    // extract high disc - group bits (9-16 Bit)
    rxDiscH_ = 0;
    for (int i = 0; i < 8; i++) {
      if (65u + i >= len || lowBuf_[65 + i] < hiBuf_[65 + i]) // not received -> 0
        rxDiscH_ &= ~(1 << i);
      else
        rxDiscH_ |= (1 << i);
    }

    // plausibility check 1: every bit = one LOW + one HIGH pulse of 400/800 us (accepted 200-1000 us),
    // period ~1200 us. Random noise (AGC of the receiver amplifies noise when
    // no transmitter is active) occasionally looks like a sync pulse and is
    // then "decoded" into garbage.
    bool pulsesOk = true;
    unsigned int badIdx = 0;
    for (unsigned int i = 1; i <= 64 && i < len; i++) {
      unsigned long period = lowBuf_[i] + hiBuf_[i];
      if (lowBuf_[i] < 200 || hiBuf_[i] < 200 || period < 900 || period > 1500) {
        pulsesOk = false;
        badIdx = i;
        break;
      }
    }

    rxKeyGen();
    uint32_t decoded = rxDecode();

    // plausibility check 2 (KeeLoq): the decrypted hopcode contains the low
    // byte of the serial number in bits 16-23. Valid only for genuine telegrams
    // with the configured manufacturer key; noise matches with 1/256.
    bool keeloqOk = (((decoded >> 16) & 0xFF) == (rxSerial_ & 0xFF));

    if (!pulsesOk || !keeloqOk) {
      rxDiscardCount_++;
      ESP_LOGD(TAG, "RX discarded (%s): serial 0x%08lX", !pulsesOk ? "pulse timing" : "KeeLoq check", (unsigned long)rxSerial_);
      if (!pulsesOk) {
        // diagnostics: sync gap, number of pulses, first failing bit and some
        // HIGH/LOW pairs in us (HIGH = carrier on) to identify the source
        // log entries are limited to 128 characters incl. timestamp - keep it short
        char pulses[40];
        int pos = 0;
        for (unsigned int i = 1; i <= 4 && i < len && pos < (int)sizeof(pulses) - 11; i++) {
          pos += snprintf(pulses + pos, sizeof(pulses) - pos, " %u/%u", hiBuf_[i], lowBuf_[i]);
        }
        ESP_LOGD(TAG, "RX diag: sync %u, %u p, bit %u %u/%u, H/L%s", lowBuf_[0], len, badIdx - 1, hiBuf_[badIdx], lowBuf_[badIdx], pulses);
      }
      rxDiscH_ = 0;
      rxHopCode_ = 0;
      rxFunction_ = 0;
      memset((void *)lowBuf_, 0, sizeof(lowBuf_));
      memset((void *)hiBuf_, 0, sizeof(hiBuf_));
      pbWrite_ = 0;
      frameReady_ = false;
      return;
    }

    if (rxFunction_ == 0x4)
      steadyCount_++;
    else
      steadyCount_--;
    if (steadyCount_ > 10 && steadyCount_ <= 40) {
      rxFunction_ = 0x3;
      steadyCount_ = 0;
    }

    rxOkCount_++;

    // build channel information
    uint8_t ch_low = (decoded >> 24) & 0xFF;
    uint8_t ch_high = rxDiscH_ & 0xFF;
    uint16_t channel = (ch_high << 8) | ch_low;

    // callback function to receive information outside this library
    remoteCallback(rxSerial_, rxFunction_, channel);

    // reset variables
    rxDiscH_ = 0;
    rxHopCode_ = 0;
    rxFunction_ = 0;
    memset((void *)lowBuf_, 0, sizeof(lowBuf_));
    memset((void *)hiBuf_, 0, sizeof(hiBuf_));
  }

  // release the buffer for the next telegram
  pbWrite_ = 0;
  frameReady_ = false;
}

/**
 * *******************************************************************
 * @brief   Setup function for jarolift controller
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::begin() {
  ESP_LOGI(TAG, "start CC1101 setup");
  EEPROM.begin(sizeof(devCount_));
  devCount_ = getDeviceCounter();

  cc1101_.setGPIO(gpio_.sck, gpio_.miso, gpio_.mosi, gpio_.cs, gpio_.gdo0);
  // TX power must be set BEFORE init(): init() writes paTableByte into PATABLE[0]
  // (single access, exactly like before). A later setTxPowerAmp() has no effect.
  cc1101_.setTxPowerAmp(PA_LongDistance);
  if (!cc1101_.init()) {
    ESP_LOGE(TAG, "Initialisation of the CC1101 module aborted!");
    return;
  }
  cc1101_.setSyncWord(kSyncWord, false);
  cc1101_.setCarrierFreq(CFREQ_433);
  cc1101_.disableAddressCheck();

  // GDO0 is driven by the RMT peripheral (exact pulse timing)
  initRmt();
  pinMode(gpio_.gdo2, INPUT_PULLUP);
  attachInterrupt(gpio_.gdo2, radioRxMeasureISR, CHANGE);

  // The CC1101 is left in IDLE after init(), so without this the receiver
  // stays deaf until the first transmission happens to enter RX as a side
  // effect - remote reception did nothing on a freshly booted device.
  enterRx();

  initOK_ = true;
}

/**
 * *******************************************************************
 * @brief   cyclic process of jarolift controller
 * @param   none
 * @return  none
 * *******************************************************************/
void JaroliftController::loop() {
  if (!initOK_)
    return;

  if (rxDataReady_) {
    cc1101_.cmdStrobe(CC1101_SCAL);
    delay(50);
    enterRx();
    rxDataReady_ = false;
    delay(200);
    attachInterrupt(gpio_.gdo2, radioRxMeasureISR, CHANGE);
  }

  processRxData(); // process received data

  // RX statistics: measured per minute, logged as an hourly summary (debug).
  // If the number of GDO2 edges or discarded telegrams exceeds a threshold
  // (e.g. a disturbing device nearby), an info message is logged once when the
  // disturbance starts and once when it has ended - not repeatedly, because on
  // some hardware the edge count is permanently above the threshold.
  if (millis() - rxStatMs_ >= 60000) {
    rxStatMs_ = millis();
    uint32_t edges = rxEdgeCount_;
    rxEdgeCount_ = 0;
    uint32_t ok = rxOkCount_;
    uint32_t discarded = rxDiscardCount_;
    rxOkCount_ = 0;
    rxDiscardCount_ = 0;
    uint8_t raw = cc1101_.readReg(CC1101_RSSI, CC1101_STATUS_REGISTER);
    int rssiDbm = ((raw >= 128) ? ((int)raw - 256) / 2 : (int)raw / 2) - 74;

    // CC1101 must be in RX (0x0D) or on its way there (0x03-0x0F: calibration,
    // settling, RX end) - anything else (IDLE, FIFO error, invalid value from a
    // hanging SPI interface) means it hangs: re-initialize
    uint8_t marcNow = cc1101_.readStatusReg(CC1101_MARCSTATE) & 0x1F;
    if (marcNow < 0x03 || marcNow > 0x0F) {
      char reason[40];
      snprintf(reason, sizeof(reason), "not in RX (MARCSTATE 0x%02X)", marcNow);
      if (reinitRadio(reason))
        enterRx();
    }

    snprintf(rxLastStat_, sizeof(rxLastStat_), "last minute: edges %lu, ok %lu, discarded %lu, RSSI %d dBm", (unsigned long)edges,
             (unsigned long)ok, (unsigned long)discarded, rssiDbm);

    // alarm
    bool alarm = (edges > kRxAlarmEdgesPerMin) || (discarded >= kRxAlarmDiscardPerMin);
    // hysteresis: the disturbance has ended after kRxAlarmClearMinutes in a row
    // clearly below the threshold, so a value around the threshold does not toggle
    bool quiet = (edges < kRxAlarmEdgesPerMin / 10 * 8) && (discarded < kRxAlarmDiscardPerMin);
    if (!rxDisturbed_) {
      if (alarm) {
        uint8_t marc = cc1101_.readStatusReg(CC1101_MARCSTATE) & 0x1F;
        ESP_LOGI(TAG, "RX disturbed: edges %lu/min, discarded %lu, RSSI %d dBm, MARC 0x%02X", (unsigned long)edges,
                 (unsigned long)discarded, rssiDbm, marc);
        rxAlarmMs_ = millis();
        rxQuietMinutes_ = 0;
        rxDisturbed_ = true;
      }
    } else if (!quiet) {
      rxQuietMinutes_ = 0;
    } else if (++rxQuietMinutes_ >= kRxAlarmClearMinutes) {
      ESP_LOGI(TAG, "RX ok again after %lu min: edges %lu/min, RSSI %d dBm", (unsigned long)((millis() - rxAlarmMs_) / 60000UL),
               (unsigned long)edges, rssiDbm);
      rxDisturbed_ = false;
    }

    // hourly summary
    rxHourEdges_ += edges;
    if (edges > rxHourEdgesMax_)
      rxHourEdgesMax_ = edges;
    rxHourOk_ += ok;
    rxHourDiscard_ += discarded;
    if (rssiDbm < rxHourRssiMin_ || rxHourMinutes_ == 0)
      rxHourRssiMin_ = rssiDbm;
    if (rssiDbm > rxHourRssiMax_ || rxHourMinutes_ == 0)
      rxHourRssiMax_ = rssiDbm;
    if (++rxHourMinutes_ >= 60) {
      ESP_LOGD(TAG, "RX stat/h: edges/min avg %lu max %lu, ok %lu, disc %lu, RSSI %d..%d", (unsigned long)(rxHourEdges_ / 60),
               (unsigned long)rxHourEdgesMax_, (unsigned long)rxHourOk_, (unsigned long)rxHourDiscard_, rxHourRssiMin_, rxHourRssiMax_);
      rxHourEdges_ = 0;
      rxHourEdgesMax_ = 0;
      rxHourOk_ = 0;
      rxHourDiscard_ = 0;
      rxHourMinutes_ = 0;
    }
  }
}
