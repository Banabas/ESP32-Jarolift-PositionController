#include <Arduino.h>
#include <Dusk2Dawn.h>
#include <config.h>
#include <jarolift.h>
#include <message.h>
#include <timer.h>
#include <shutter_position.h>

static time_t lastProcessedTime = -1;
static const char *TAG = "TIMER";

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool isDayEnabled(const s_channel_timer &ct, int day) {
  switch (day) {
    case 0: return ct.sunday;
    case 1: return ct.monday;
    case 2: return ct.tuesday;
    case 3: return ct.wednesday;
    case 4: return ct.thursday;
    case 5: return ct.friday;
    case 6: return ct.saturday;
    default: return false;
  }
}

// ─── Sunrise / Sunset ─────────────────────────────────────────────────────────

void getSunriseOrSunset(uint8_t type, int16_t offset, float latitude, float longitude,
                        uint8_t &hour, uint8_t &minute,
                        uint8_t astro_mode, int8_t horizon_value) {
  float zenith;
  switch (astro_mode) {
    case ASTRO_CIVIL:       zenith = ZENITH_CIVIL;                 break;
    case ASTRO_NAUTIC:      zenith = ZENITH_NAUTIC;                break;
    case ASTRO_ASTRONOMIC:  zenith = ZENITH_ASTRONOMIC;            break;
    case ASTRO_HORIZON:     zenith = 90.0f - (float)horizon_value; break;
    default:                zenith = ZENITH_REAL;                  break;
  }

  time_t now; tm dti, utcTime;
  time(&now);
  localtime_r(&now, &dti);
  gmtime_r(&now, &utcTime);

  float utcOffset = static_cast<float>(difftime(mktime(&dti), mktime(&utcTime))) / 3600.0f;
  Dusk2Dawn location(latitude, longitude, utcOffset);

  int16_t timeInMinutes;
  if (type == TYPE_SUNRISE) {
    timeInMinutes = location.sunrise(dti.tm_year + 1900, dti.tm_mon + 1, dti.tm_mday, dti.tm_isdst, zenith);
  } else if (type == TYPE_SUNDOWN) {
    timeInMinutes = location.sunset(dti.tm_year + 1900, dti.tm_mon + 1, dti.tm_mday, dti.tm_isdst, zenith);
  } else {
    hour = 0; minute = 0; return;
  }

  timeInMinutes += offset;
  timeInMinutes  = (timeInMinutes + 1440) % 1440;
  hour   = timeInMinutes / 60;
  minute = timeInMinutes % 60;
}

// ─── Time parsing ─────────────────────────────────────────────────────────────

static int getHour(const char *t)   { return strlen(t) == 5 ? (t[0]-'0')*10 + (t[1]-'0') : -1; }
static int getMinute(const char *t) { return strlen(t) == 5 ? (t[3]-'0')*10 + (t[4]-'0') : -1; }

// ─── Event trigger check ─────────────────────────────────────────────────────

static void calcEventTime(uint8_t type, const char *timeVal, int16_t offset,
                           uint8_t astroMode, int8_t horizonVal,
                           bool useMin, const char *minTime,
                           bool useMax, const char *maxTime,
                           uint8_t &evH, uint8_t &evM) {
  if (type == TYPE_FIXED_TIME) {
    evH = getHour(timeVal);
    evM = getMinute(timeVal);
  } else {
    getSunriseOrSunset(type, offset, config.geo.latitude, config.geo.longitude,
                       evH, evM, astroMode, horizonVal);
    if (useMin) {
      uint8_t mnH = getHour(minTime), mnM = getMinute(minTime);
      if (evH < mnH || (evH == mnH && evM < mnM)) { evH = mnH; evM = mnM; }
    }
    if (useMax) {
      uint8_t mxH = getHour(maxTime), mxM = getMinute(maxTime);
      if (evH > mxH || (evH == mxH && evM > mxM)) { evH = mxH; evM = mxM; }
    }
  }
}

static bool eventTriggered(const s_timer_event &ev, uint8_t curH, uint8_t curM, int wday) {
  if (!ev.enable) return false;

  bool isWeekend = (wday == 0 || wday == 6); // 0=Sunday, 6=Saturday

  uint8_t evH, evM;

  if (isWeekend && ev.weekend_enable) {
    // Use weekend-specific time
    calcEventTime(ev.weekend_type, ev.weekend_time_value, ev.weekend_offset_value,
                  ev.weekend_astro_mode, ev.weekend_horizon_value,
                  false, "", false, "", evH, evM);
  } else {
    // Use normal weekday time
    calcEventTime(ev.type, ev.time_value, ev.offset_value,
                  ev.astro_mode, ev.horizon_value,
                  ev.use_min_time, ev.min_time_value,
                  ev.use_max_time, ev.max_time_value,
                  evH, evM);
  }

  return (evH == curH && evM == curM);
}

// ─── Timer cyclic ─────────────────────────────────────────────────────────────

void timerCyclic() {
  time_t now; tm dti;
  time(&now);
  localtime_r(&now, &dti);

  if (dti.tm_year < 125) return; // wait for valid NTP time

  if ((now / 60) == (lastProcessedTime / 60)) return; // once per minute
  lastProcessedTime = now;

  uint8_t H = dti.tm_hour;
  uint8_t M = dti.tm_min;
  int     W = dti.tm_wday;

  const bool isWeekend = (W == 0 || W == 6);

  // ── Per-channel timers ────────────────────────────────────────────────────
  for (int ch = 0; ch < 16; ch++) {
    const s_channel_timer &ct = config.ch_timer[ch];
    if (!ct.enable || !config.jaro.ch_enable[ch]) continue;
    const bool dayOk = isDayEnabled(ct, W);

    if (ct.up.enable && (dayOk || (isWeekend && ct.up.weekend_enable)) &&
        eventTriggered(ct.up, H, M, W)) {
      ESP_LOGI(TAG, "Ch-Timer ch %d UP", ch + 1);
      jaroCmd(CMD_UP, (uint8_t)ch);
    }
    if (ct.down.enable && (dayOk || (isWeekend && ct.down.weekend_enable)) &&
        eventTriggered(ct.down, H, M, W)) {
      ESP_LOGI(TAG, "Ch-Timer ch %d DOWN", ch + 1);
      jaroCmd(CMD_DOWN, (uint8_t)ch);
    }
  }

  // ── Per-group timers ──────────────────────────────────────────────────────
  for (int g = 0; g < 6; g++) {
    const s_channel_timer &gt = config.grp_timer[g];
    if (!gt.enable || !config.jaro.grp_enable[g]) continue;
    const bool dayOk = isDayEnabled(gt, W);

    if (gt.up.enable && (dayOk || (isWeekend && gt.up.weekend_enable)) &&
        eventTriggered(gt.up, H, M, W)) {
      ESP_LOGI(TAG, "Grp-Timer grp %d UP", g + 1);
      jaroCmd(CMD_GRP_UP, config.jaro.grp_mask[g]);
    }
    if (gt.down.enable && (dayOk || (isWeekend && gt.down.weekend_enable)) &&
        eventTriggered(gt.down, H, M, W)) {
      ESP_LOGI(TAG, "Grp-Timer grp %d DOWN", g + 1);
      jaroCmd(CMD_GRP_DOWN, config.jaro.grp_mask[g]);
    }
  }
}
