/**
 * shutter_position.h
 * ==================
 * Time-based position tracking for Jarolift TDEF roller shutters.
 *
 * Position convention
 *   0%   = fully closed (bottom mechanical end-stop)
 *   100% = fully open    (top mechanical end-stop)
 *   (matches Home Assistant's cover position convention)
 *
 * Stopping mechanism
 *   Stop commands are issued via jaroStopNow() which bypasses the 500ms
 *   command queue and sends the RF telegram immediately.  The stop time is
 *   calculated as an absolute wall-clock timestamp (millis() + travel_ms)
 *   so the accuracy does not depend on task scheduling jitter.
 *
 * Calibration – two phases
 *   Phase 1 (DOWN): shutter at 100% → shutterCalibDownStart() → CMD_DOWN
 *                   → shutter at 0%   → CMD_STOP → shutterCalibDownFinish()
 *   Phase 2 (UP):   shutter at 0%   → shutterCalibUpStart() → CMD_UP
 *                   → shutter at 100% → CMD_STOP → shutterCalibUpFinish()
 *                   → configSaveToFile()
 *
 * If only Phase 1 is done, UP travel is estimated as DOWN × 1.2.
 */

#pragma once
#include <stdint.h>
#include <jarolift.h>

void mqttSendPosition(uint8_t channel, uint8_t position);  // defined in jarolift.cpp

void     shutterPosInit();
void     shutterPosUpdate(void (*stopCb)(JaroCmdType, uint8_t));  // stopCb unused but kept for API compat

bool     shutterPosSetTarget(uint8_t ch, int8_t targetPct,
                             void (*stopCb)(JaroCmdType, uint8_t),
                             void (*upCb)(JaroCmdType, uint8_t),
                             void (*downCb)(JaroCmdType, uint8_t));

void     shutterPosNotifyUp(uint8_t ch);
void     shutterPosNotifyDown(uint8_t ch);
void     shutterPosNotifyStop(uint8_t ch);

void     shutterPosForceOpen(uint8_t ch);
void     shutterPosForceClose(uint8_t ch);
int8_t   shutterPosGet(uint8_t ch);
bool     shutterIsMoving(uint8_t ch);   ///< true if motor is currently running

// Calibration – two phase
void     shutterCalibDownStart(uint8_t ch);
uint32_t shutterCalibDownFinish(uint8_t ch);
void     shutterCalibUpStart(uint8_t ch);
uint32_t shutterCalibUpFinish(uint8_t ch);
bool     shutterCalibIsRunning(uint8_t ch);

// Manual correction of measured travel times (ms) from the WebUI
void     shutterSetTravelTime(uint8_t ch, uint32_t downMs, uint32_t upMs);

// Legacy aliases (DOWN only)
void     shutterCalibStart(uint8_t ch);
uint32_t shutterCalibFinish(uint8_t ch);
