// JaroliftController.h
#ifndef JARO_LIFT_CONTROLLER_H
#define JARO_LIFT_CONTROLLER_H

#include "KeeloqLib.h"
#include "cc1101.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <nvs.h>
#include <nvs_flash.h>

class JaroliftController {
public:
  // Kapselung der GPIO- und Konfigurationsdaten
  struct GPIO {
    int sck;
    int miso;
    int mosi;
    int cs;
    int gdo0; // TX
    int gdo2; // RX
  };

  struct Config {
    uint32_t serial;
    bool learnMode;
    unsigned long masterMSB;
    unsigned long masterLSB;
  };

  // enum of shutter commands
  enum commands {
    CMD_UP,
    CMD_DOWN,
    CMD_STOP,
    CMD_SHADE,
    CMD_SET_SHADE,
  };

  JaroliftController();
  ~JaroliftController();

  // Lebenszyklus
  void begin();
  void loop();

  // Einzelbefehle
  void cmdChannel(commands cmd, uint8_t channel);

  // Gruppenbefehle
  void cmdGroup(commands cmd, uint16_t groupMask);

  // Service Commands
  void cmdLearn(uint8_t channel);
  void cmdUnlearn(uint8_t channel);
  void cmdSetEndPointUp(uint8_t channel);
  void cmdDeleteEndPointUp(uint8_t channel);
  void cmdSetEndPointDown(uint8_t channel);
  void cmdDeleteEndPointDown(uint8_t channel);

  // Konfigurationssetter
  void setGPIO(int sck, int miso, int mosi, int cs, int gdo0, int gdo2);
  void setBaseSerial(uint32_t serial);
  void setLegacyLearnMode(bool legacyMode);
  void setKeys(unsigned long masterMSB, unsigned long masterLSB);
  void setRemoteCallback(void (*callback)(uint32_t serial, int8_t function, uint16_t channel)) { remoteCallback = callback; }

  // Hilfsfunktionen
  uint16_t getDeviceCounter();
  void setDeviceCounter(uint16_t newDevCnt);
  uint32_t getSerial(uint8_t channel);
  bool getCC1101State();
  uint8_t getRssi();
  // direct CC1101 register access for tests (telnet "cc1101"), not persistent
  uint8_t ccReadReg(uint8_t reg);
  void ccWriteReg(uint8_t reg, uint8_t value);
  const char *getRxStat() { return rxLastStat_; } // values of the last full minute
  // millis() when the first telegram of the last transmission was complete
  // (= the motor starts), 0 if nothing was sent
  uint32_t getTxStartMs() const { return txStartMs_; }

private:
  // Instanzvariablen (anstatt globaler Variablen)
  GPIO gpio_;
  Config config_;
  uint16_t devCount_;

  int deviceKeyMSB_;
  int deviceKeyLSB_;

  uint64_t button_;
  uint8_t discL_;
  uint8_t discH_;
  uint16_t disc_;

  uint64_t newSerial_;

  uint32_t encrypted_;
  uint64_t pack_;

  // Empfangspuffer
  static constexpr size_t kPulseBufferSize = 216;
  volatile unsigned int lowBuf_[kPulseBufferSize];
  volatile unsigned int hiBuf_[kPulseBufferSize];
  volatile unsigned int pbWrite_;
  volatile bool frameReady_; // complete telegram in the buffer, RX paused until processRxData()
  static constexpr unsigned int kMinTelegram = 65;  // pulses up to the function code
  static constexpr unsigned int kFullTelegram = 73; // sync + 72 bits incl. group bits 9-16
  static bool isSync(unsigned int lowVal) { return lowVal > 3650 && lowVal < 4300; }

  // Empfangsdaten
  uint32_t rxSerial_;
  uint32_t rxHopCode_;
  uint8_t rxFunction_;
  uint16_t rxDiscH_;

  // Laufzeitzustand
  bool initOK_;
  unsigned int steadyCount_;
  bool rxDataReady_;

  // Hardware-Modul
  CC1101 cc1101_;

  // Konstanten
  static constexpr int kLowPulse = 400; // in µs
  static constexpr int kHighPulse = 800;
  static constexpr int kTxRepeat = 5;     // repetitions for UP/DOWN (same counter -> executed once)
  static constexpr int kTxRepeatStop = 2; // STOP unchanged: a held STOP is a command of its own (shade)
  static constexpr int kDebounce = 200;
  static constexpr uint8_t kSyncWord = 199;

  static constexpr uint8_t FCT_CODE_UP = 0x8;
  static constexpr uint8_t FCT_CODE_DOWN = 0x2;
  static constexpr uint8_t FCT_CODE_STOP = 0x4;
  static constexpr uint8_t FCT_CODE_UPDOWN = 0xA;

  static constexpr char *TAG = "JARO-LIB"; // LOG TAG

  // Diskriminierungsarrays (analog zu den Originalwerten)
  uint8_t discLowArr_[16];
  uint8_t discHighArr_[16];

  // Hilfsfunktionen
  void updateDeviceCounter(bool increment);

  // TX via RMT (hardware timing, independent of WiFi/task interrupts)
  static constexpr uint32_t kRmtResolutionHz = 1000000; // 1 tick = 1 µs
  static constexpr size_t kMaxTxSymbols = 96;           // telegram needs 86 symbols
  rmt_data_t txSymbols_[kMaxTxSymbols];
  size_t txSymbolCount_;
  bool txHalfUsed_;
  bool rmtOK_;
  // RX statistics (debug log once per minute)
  volatile uint32_t rxEdgeCount_ = 0;
  uint32_t rxOkCount_ = 0;
  uint32_t rxDiscardCount_ = 0;
  uint32_t rxStatMs_ = 0;
  // hourly summary + alarm when thresholds are exceeded
  uint32_t rxHourEdges_ = 0;
  uint32_t rxHourEdgesMax_ = 0;
  uint32_t rxHourOk_ = 0;
  uint32_t rxHourDiscard_ = 0;
  int rxHourRssiMin_ = 0;
  int rxHourRssiMax_ = -200;
  uint8_t rxHourMinutes_ = 0;
  uint32_t rxAlarmMs_ = 0;
  bool rxDisturbed_ = false;   // "RX disturbed" was logged, waiting for the end of the disturbance
  uint8_t rxQuietMinutes_ = 0; // minutes in a row clearly below the alarm threshold
  char rxLastStat_[96] = "no data yet";
  static constexpr uint32_t kRxAlarmEdgesPerMin = 50000; // normal: a few 1000/min
  static constexpr uint32_t kRxAlarmDiscardPerMin = 5;
  static constexpr uint8_t kRxAlarmClearMinutes = 5; // quiet minutes until "RX ok again" is logged
  uint32_t txStartMs_;
  void initRmt();
  bool reinitRadio(const char *reason);
  void txAddLevel(uint8_t level, uint16_t durationUs);
  void radioTxFrame(int length);
  void radioTxGroupH();
  void radioTx(int repetitions);
  void enterRx();
  void enterTx();
  void processRxData();
  void generateKey();       // Schlüsselgenerierung (keygen)
  void generateEncrypted(); // Verschlüsseln (keeloq)

  void rxKeyGen();
  uint32_t rxDecode();

  void (*remoteCallback)(uint32_t serial, int8_t function, uint16_t channel) = nullptr;

  // Interrupt-Service-Routine (ISR) für RX-Messung
  static void IRAM_ATTR radioRxMeasureISR();
  void handleRadioRxMeasure();

  // Singleton-Zeiger für den ISR-Zugriff
  static JaroliftController *instance_;
};

#endif // JARO_LIFT_CONTROLLER_H
