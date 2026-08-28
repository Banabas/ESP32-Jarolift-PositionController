#pragma once
#include <message.h>
#include <stdint.h>
/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION "v1.21.4" // internal program version

#define WIFI_RECONNECT 30000 // Delay between wifi reconnection tries
#define MQTT_RECONNECT 10000 // Delay between mqtt reconnection tries

struct s_cfg_jaro {
  unsigned long masterMSB;
  unsigned long masterLSB;
  bool learn_mode;
  uint32_t serial;
  char ch_name[16][64]{"\0"};
  uint32_t ch_travel_time[16];    // calibrated DOWN travel time in ms (0 = uncalibrated)
  uint32_t ch_travel_time_up[16]; // calibrated UP travel time in ms (0 = use DOWN * UP_FACTOR)
  int8_t ch_last_pos[16];       // zuletzt bekannte Position 0-100
  bool ch_enable[16];
  char grp_name[6][64]{"\0"};
  bool grp_enable[6];
  uint16_t grp_mask[6];
  char remote_name[16][64]{"\0"};
  uint32_t remote_serial[16];
  bool remote_enable[16];
  uint16_t remote_mask[16];
};

// Astro modes for sunrise/sunset timer
#define ASTRO_REAL        0  // standard sunrise/sunset (zenith 90.833)
#define ASTRO_CIVIL       1  // civil twilight         (zenith 96)
#define ASTRO_NAUTIC      2  // nautical twilight      (zenith 102)
#define ASTRO_ASTRONOMIC  3  // astronomical twilight  (zenith 108)
#define ASTRO_HORIZON     4  // custom horizon angle

// Timer event: one time trigger (up or down)
struct s_timer_event {
  bool    enable;
  uint8_t type;            // 0=fixed, 1=sunrise, 2=sunset
  char    time_value[6];   // HH:MM
  int16_t offset_value;    // minutes offset (-20..+20)
  uint8_t astro_mode;      // 0=REAL..4=HORIZON
  int8_t  horizon_value;   // -9..+9 degrees (HORIZON mode only)
  bool    use_min_time;
  char    min_time_value[6];
  bool    use_max_time;
  char    max_time_value[6];
  // Weekend override (Sa+So)
  bool    weekend_enable;
  uint8_t weekend_type;
  char    weekend_time_value[6];
  int16_t weekend_offset_value;
  uint8_t weekend_astro_mode;
  int8_t  weekend_horizon_value;
};

// Per-channel/group timer: up event + down event + weekdays
struct s_channel_timer {
  bool          enable;
  s_timer_event up;
  s_timer_event down;
  bool monday;
  bool tuesday;
  bool wednesday;
  bool thursday;
  bool friday;
  bool saturday;
  bool sunday;
};

// Legacy generic timer (kept for backward compat, unused)
struct s_cfg_timer {
  bool enable;
  uint8_t type;
  char time_value[6];
  int16_t offset_value;
  uint8_t astro_mode;
  int8_t  horizon_value;
  uint8_t cmd;
  bool monday, tuesday, wednesday, thursday, friday, saturday, sunday;
  uint16_t grp_mask;
  bool use_min_time;
  char min_time_value[6];
  bool use_max_time;
  char max_time_value[6];
};

struct s_cfg_geo {
  float latitude;
  float longitude;
};

struct s_cfg_wifi {
  bool enable = false;
  char ssid[128];
  char password[128];
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
};

struct s_cfg_eth {
  bool enable = false;
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
  int gpio_sck;
  int gpio_mosi;
  int gpio_miso;
  int gpio_cs;
  int gpio_irq;
  int gpio_rst;
};

struct s_cfg_mqtt {
  bool enable;
  char server[128];
  char user[128];
  char password[128];
  char topic[128];
  uint16_t port = 1883;
  bool ha_enable;
  char ha_topic[64];
  char ha_device[32];
};

struct s_cfg_ntp {
  bool enable = true;
  char server[128] = {"de.pool.ntp.org"};
  char tz[128] = {"CET-1CEST,M3.5.0,M10.5.0/3"};
};

struct s_cfg_gpio {
  int led_setup;
  int gdo0; // TX
  int gdo2; // RX
  int sck;
  int mosi;
  int miso;
  int cs;
};

struct s_cfg_auth {
  bool enable = true;
  char user[64];
  char password[64];
};

struct s_cfg_log {
  bool enable = true;
  int level = 3;
  int order = 0;
};

struct s_config {
  int version;
  int lang;
  s_cfg_wifi wifi;
  s_cfg_eth eth;
  s_cfg_mqtt mqtt;
  s_cfg_ntp ntp;
  s_cfg_gpio gpio;
  s_cfg_auth auth;
  s_cfg_log log;
  s_cfg_jaro jaro;
  s_cfg_timer    timer[6];      // legacy (unused)
  s_channel_timer ch_timer[16]; // per-channel timer
  s_channel_timer grp_timer[6]; // per-group timer
  s_cfg_geo geo;
};

extern s_config config;
extern bool setupMode;
void configSetup();
void configCyclic();
void configSaveToFile();
void configLoadFromFile();
void configInitValue();
void configGPIO();