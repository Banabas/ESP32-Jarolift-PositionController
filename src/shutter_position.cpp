/**
 * shutter_position.cpp
 * ====================
 * Time-based position tracking for Jarolift TDEF roller shutters.
 *
 * How it works
 * ------------
 * 1. shutterPosSetTarget() stores the target and queues CMD_DOWN/UP normally.
 * 2. When the queue processes CMD_DOWN/UP it calls shutterPosNotifyDown/Up().
 *    THIS is the moment the motor actually starts – here we calculate the
 *    absolute stop timestamp:  stopAtMs = millis() + (pct_span * travel_ms / 100)
 * 3. shutterPosUpdate() is called every 100ms. When millis() >= stopAtMs it
 *    calls jaroStopNow() which sends the RF STOP immediately (no queue delay).
 *
 * Position convention
 *   0%   = fully closed (bottom end-stop)
 *   100% = fully open    (top end-stop)
 *   (matches Home Assistant's cover position convention)
 *
 * Calibration
 *   DOWN travel time: measured from actual motor start (NotifyDown) to stop.
 *   UP   travel time: measured from actual motor start (NotifyUp)   to stop.
 *   If only DOWN is calibrated, UP = DOWN × UP_FACTOR_DEFAULT (fallback).
 */

#include "shutter_position.h"
#include <config.h>
#include <Arduino.h>

extern void jaroStopNow(uint8_t channel);  // defined in jarolift.cpp

static const char *TAG = "POS";

// ─── Constants ────────────────────────────────────────────────────────────────

static constexpr float   UP_FACTOR_DEFAULT = 1.2f;
static constexpr uint8_t CH_COUNT          = 16;

// ─── State ────────────────────────────────────────────────────────────────────

struct ShutterState {
    int8_t   pos;         ///< Last known / estimated position 0–100
    int8_t   posAtStart;  ///< Position when current RF cmd was sent
    int8_t   target;      ///< Target position; -1 = run to end-stop
    uint32_t startMs;     ///< millis() when RF cmd was actually transmitted
    uint32_t stopAtMs;    ///< Absolute time to fire jaroStopNow (0 = no timer)
    bool     moving;
    bool     goingDown;
    bool     pendingMove; ///< CMD queued, waiting for NotifyDown/Up to start timer
};

struct CalibState {
    bool     activeDown;
    bool     activeUp;
    uint32_t startMs;    ///< Set in NotifyDown/Up so queue delay is excluded
};

static ShutterState s[CH_COUNT];
static CalibState   c[CH_COUNT];

// ─── Helpers ──────────────────────────────────────────────────────────────────

static inline int32_t clamp32(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/** Channel label for log output: configured name, or "Ch N" fallback. */
static const char *chName(uint8_t ch) {
    static char buf[24];
    if (ch < CH_COUNT && config.jaro.ch_name[ch][0]) return config.jaro.ch_name[ch];
    snprintf(buf, sizeof(buf), "Ch %u", ch + 1u);
    return buf;
}

/** Effective travel time for the given direction. */
static uint32_t travelTime(uint8_t ch, bool goingDown) {
    if (goingDown)
        return config.jaro.ch_travel_time[ch];
    const uint32_t up = config.jaro.ch_travel_time_up[ch];
    return up > 0 ? up
                  : static_cast<uint32_t>(config.jaro.ch_travel_time[ch] * UP_FACTOR_DEFAULT);
}

/** Calculate estimated position from elapsed time. No side-effects. */
static int8_t calcPos(uint8_t ch) {
    if (!s[ch].moving || config.jaro.ch_travel_time[ch] == 0 || s[ch].startMs == 0)
        return s[ch].pos;
    const uint32_t elapsed = millis() - s[ch].startMs;
    const uint32_t travel  = travelTime(ch, s[ch].goingDown);
    const int32_t  pct     = clamp32(
        static_cast<int32_t>(static_cast<uint64_t>(elapsed) * 100u / travel), 0, 100);
    // DOWN closes (position decreases toward 0), UP opens (position increases toward 100)
    const int32_t  pos = s[ch].goingDown ? s[ch].posAtStart - pct
                                         : s[ch].posAtStart + pct;
    return static_cast<int8_t>(clamp32(pos, 0, 100));
}

/** Immediately mark channel as stationary at pos. */
static void forcePos(uint8_t ch, int8_t pos) {
    s[ch].pos         = pos;
    s[ch].moving      = false;
    s[ch].target      = -1;
    s[ch].stopAtMs    = 0;
    s[ch].pendingMove = false;
    config.jaro.ch_last_pos[ch] = pos;
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

void shutterPosInit() {
    for (uint8_t i = 0; i < CH_COUNT; ++i) {
        s[i] = { config.jaro.ch_last_pos[i], config.jaro.ch_last_pos[i],
                 -1, 0, 0, false, false, false };
        c[i] = { false, false, 0 };
    }
    ESP_LOGI(TAG, "position tracking ready (%u channels)", CH_COUNT);
}

// ─── Cyclic update ────────────────────────────────────────────────────────────

void shutterPosUpdate(void (* /*stopCb*/)(JaroCmdType, uint8_t)) {
    const uint32_t now = millis();
    for (uint8_t ch = 0; ch < CH_COUNT; ++ch) {
        if (!s[ch].moving) continue;
        if (config.jaro.ch_travel_time[ch] == 0) continue;

        if (s[ch].stopAtMs == 0) {
            // End-stop move: lock position once full travel time has elapsed
            const int8_t est = calcPos(ch);
            if (s[ch].goingDown && est <= 0) {
                forcePos(ch, 0);
                mqttSendPosition(ch, 0);
                ESP_LOGI(TAG, "%s: bottom end-stop reached (closed)", chName(ch));
            } else if (!s[ch].goingDown && est >= 100) {
                forcePos(ch, 100);
                mqttSendPosition(ch, 100);
                ESP_LOGI(TAG, "%s: top end-stop reached (open)", chName(ch));
            }
            continue;
        }

        // Timed stop: fire immediately when wall-clock time is reached
        if (now >= s[ch].stopAtMs) {
            const int8_t fixPos = s[ch].target;
            jaroStopNow(ch);
            s[ch].pos         = fixPos;
            s[ch].moving      = false;
            s[ch].target      = -1;
            s[ch].stopAtMs    = 0;
            s[ch].pendingMove = false;
            config.jaro.ch_last_pos[ch] = fixPos;
            mqttSendPosition(ch, static_cast<uint8_t>(fixPos));
            ESP_LOGI(TAG, "%s: stopped at %d%%", chName(ch), fixPos);
        }
    }
}

// ─── Position command ─────────────────────────────────────────────────────────

bool shutterPosSetTarget(uint8_t ch, int8_t targetPct,
                         void (*stopCb)(JaroCmdType, uint8_t),
                         void (*upCb)(JaroCmdType, uint8_t),
                         void (*downCb)(JaroCmdType, uint8_t)) {
    if (ch >= CH_COUNT) return false;
    if (config.jaro.ch_travel_time[ch] == 0) {
        ESP_LOGW(TAG, "%s: not calibrated – ignoring position command", chName(ch));
        return false;
    }

    targetPct = static_cast<int8_t>(clamp32(targetPct, 0, 100));

    if (s[ch].moving) s[ch].pos = calcPos(ch);
    if (s[ch].pos == targetPct && !s[ch].moving) return true;

    // Lower target % = more closed → must go DOWN. Higher target % = more open → go UP.
    const bool needsDown = (targetPct < s[ch].pos);

    // Same direction: recalculate stop time immediately from current estimated position
    if (s[ch].moving && s[ch].goingDown == needsDown) {
        s[ch].target = (targetPct == 0 || targetPct == 100) ? -1 : targetPct;
        if (s[ch].target >= 0) {
            const uint32_t travel  = travelTime(ch, needsDown);
            const uint32_t pctSpan = needsDown
                ? static_cast<uint32_t>(s[ch].pos    - s[ch].target)
                : static_cast<uint32_t>(s[ch].target - s[ch].pos);
            s[ch].stopAtMs = millis() + (pctSpan * travel / 100u);
        } else {
            s[ch].stopAtMs = 0;
        }
        s[ch].pendingMove = false;
        ESP_LOGI(TAG, "%s: retarget → %d%% (same dir)", chName(ch), targetPct);
        return true;
    }

    // Direction change or fresh start: stop first
    if (s[ch].moving) {
        s[ch].stopAtMs    = 0;
        stopCb(CMD_STOP, ch);
        s[ch].moving      = false;
        s[ch].pendingMove = false;
    }

    s[ch].target      = (targetPct == 0 || targetPct == 100) ? -1 : targetPct;
    s[ch].pendingMove = true;
    s[ch].goingDown   = needsDown;

    needsDown ? downCb(CMD_DOWN, ch) : upCb(CMD_UP, ch);
    ESP_LOGI(TAG, "%s → %d%% queued (%s)", chName(ch), targetPct,
             needsDown ? "DOWN" : "UP");
    return true;
}

// ─── Notify (called when RF command is actually transmitted) ──────────────────

void shutterPosNotifyUp(uint8_t ch) {
    if (ch >= CH_COUNT) return;
    if (s[ch].moving) s[ch].pos = calcPos(ch);

    s[ch].posAtStart = s[ch].pos;
    s[ch].startMs    = millis();
    s[ch].goingDown  = false;
    s[ch].moving     = true;

    // Start calibration stopwatch now (queue delay excluded)
    if (c[ch].activeUp && c[ch].startMs == 0) {
        c[ch].startMs = s[ch].startMs;
        ESP_LOGI(TAG, "%s: UP calib timer started", chName(ch));
    }

    // UP opens: position increases, so a valid target must be greater than current pos
    if (s[ch].pendingMove && s[ch].target >= 0) {
        const uint32_t travel  = travelTime(ch, false);
        if (s[ch].target <= s[ch].pos) {
            // Position estimate drifted past target; fire stop immediately
            s[ch].stopAtMs = millis();
        } else {
            const uint32_t pctSpan = static_cast<uint32_t>(s[ch].target - s[ch].pos);
            const uint32_t stopMs  = pctSpan * travel / 100u;
            s[ch].stopAtMs = s[ch].startMs + stopMs;
            ESP_LOGI(TAG, "%s: UP from %d%% → %d%%, stop in %lu ms",
                     chName(ch), s[ch].pos, s[ch].target, stopMs);
        }
        s[ch].pendingMove = false;
    } else {
        s[ch].stopAtMs    = 0;
        s[ch].pendingMove = false;
        if (s[ch].target >= 0 && s[ch].target <= s[ch].pos)
            s[ch].target = -1;
        ESP_LOGI(TAG, "%s: UP from %d%% (running to top end-stop)", chName(ch), s[ch].pos);
    }
}

void shutterPosNotifyDown(uint8_t ch) {
    if (ch >= CH_COUNT) return;
    if (s[ch].moving) s[ch].pos = calcPos(ch);

    s[ch].posAtStart = s[ch].pos;
    s[ch].startMs    = millis();
    s[ch].goingDown  = true;
    s[ch].moving     = true;

    // Start calibration stopwatch now (queue delay excluded)
    if (c[ch].activeDown && c[ch].startMs == 0) {
        c[ch].startMs = s[ch].startMs;
        ESP_LOGI(TAG, "%s: DOWN calib timer started", chName(ch));
    }

    // DOWN closes: position decreases, so a valid target must be less than current pos
    if (s[ch].pendingMove && s[ch].target >= 0) {
        const uint32_t travel  = travelTime(ch, true);
        if (s[ch].target >= s[ch].pos) {
            // Position estimate drifted past target; fire stop immediately
            s[ch].stopAtMs = millis();
        } else {
            const uint32_t pctSpan = static_cast<uint32_t>(s[ch].pos - s[ch].target);
            const uint32_t stopMs  = pctSpan * travel / 100u;
            s[ch].stopAtMs = s[ch].startMs + stopMs;
            ESP_LOGI(TAG, "%s: DOWN from %d%% → %d%%, stop in %lu ms",
                     chName(ch), s[ch].pos, s[ch].target, stopMs);
        }
        s[ch].pendingMove = false;
    } else {
        s[ch].stopAtMs    = 0;
        s[ch].pendingMove = false;
        if (s[ch].target >= 0 && s[ch].target >= s[ch].pos)
            s[ch].target = -1;
        ESP_LOGI(TAG, "%s: DOWN from %d%% (running to bottom end-stop)", chName(ch), s[ch].pos);
    }
}

void shutterPosNotifyStop(uint8_t ch) {
    if (ch >= CH_COUNT) return;
    if (s[ch].moving) s[ch].pos = calcPos(ch);
    s[ch].moving      = false;
    s[ch].target      = -1;
    s[ch].stopAtMs    = 0;
    s[ch].pendingMove = false;
    config.jaro.ch_last_pos[ch] = s[ch].pos;
    ESP_LOGI(TAG, "%s: stopped at %d%%", chName(ch), s[ch].pos);
}

// ─── Position overrides ───────────────────────────────────────────────────────

void   shutterPosForceOpen(uint8_t ch)  { if (ch < CH_COUNT) forcePos(ch, 100); }
void   shutterPosForceClose(uint8_t ch) { if (ch < CH_COUNT) forcePos(ch, 0);   }
int8_t shutterPosGet(uint8_t ch)        { return ch < CH_COUNT ? calcPos(ch) : -1; }
bool   shutterIsMoving(uint8_t ch)      { return ch < CH_COUNT && s[ch].moving; }

// ─── Calibration ──────────────────────────────────────────────────────────────

void shutterCalibDownStart(uint8_t ch) {
    if (ch >= CH_COUNT) return;
    forcePos(ch, 100); // assume fully open before measuring the closing (DOWN) run
    c[ch].startMs    = 0;       // set in NotifyDown when motor actually starts
    c[ch].activeDown = true;
    c[ch].activeUp   = false;
    ESP_LOGI(TAG, "%s: DOWN calib armed", chName(ch));
}

uint32_t shutterCalibDownFinish(uint8_t ch) {
    if (ch >= CH_COUNT || !c[ch].activeDown) return 0;
    if (c[ch].startMs == 0) {
        c[ch].activeDown = false;
        ESP_LOGW(TAG, "%s: DOWN calib aborted (motor did not start)", chName(ch));
        return 0;
    }
    const uint32_t ms = millis() - c[ch].startMs;
    c[ch].activeDown = false;
    config.jaro.ch_travel_time[ch] = ms;
    forcePos(ch, 0); // fully closed
    ESP_LOGI(TAG, "%s: DOWN calib done – %lu ms (%.1f s)", chName(ch), ms, ms / 1000.0f);
    return ms;
}

void shutterCalibUpStart(uint8_t ch) {
    if (ch >= CH_COUNT) return;
    forcePos(ch, 0); // assume fully closed before measuring the opening (UP) run
    c[ch].startMs    = 0;       // set in NotifyUp when motor actually starts
    c[ch].activeUp   = true;
    c[ch].activeDown = false;
    ESP_LOGI(TAG, "%s: UP calib armed", chName(ch));
}

uint32_t shutterCalibUpFinish(uint8_t ch) {
    if (ch >= CH_COUNT || !c[ch].activeUp) return 0;
    if (c[ch].startMs == 0) {
        c[ch].activeUp = false;
        ESP_LOGW(TAG, "%s: UP calib aborted (motor did not start)", chName(ch));
        return 0;
    }
    const uint32_t ms = millis() - c[ch].startMs;
    c[ch].activeUp = false;
    config.jaro.ch_travel_time_up[ch] = ms;
    forcePos(ch, 100); // fully open
    ESP_LOGI(TAG, "%s: UP calib done – %lu ms (%.1f s)", chName(ch), ms, ms / 1000.0f);
    return ms;
}

bool shutterCalibIsRunning(uint8_t ch) {
    return ch < CH_COUNT && (c[ch].activeDown || c[ch].activeUp);
}

/** Manual correction of measured travel times (in ms) from the WebUI. */
void shutterSetTravelTime(uint8_t ch, uint32_t downMs, uint32_t upMs) {
    if (ch >= CH_COUNT) return;
    config.jaro.ch_travel_time[ch]    = downMs;
    config.jaro.ch_travel_time_up[ch] = upMs;
    ESP_LOGI(TAG, "%s: travel time manually set – DOWN %lu ms, UP %lu ms",
             chName(ch), downMs, upMs);
}

// Legacy aliases (single-phase DOWN only)
void     shutterCalibStart(uint8_t ch)  { shutterCalibDownStart(ch); }
uint32_t shutterCalibFinish(uint8_t ch) { return shutterCalibDownFinish(ch); }
