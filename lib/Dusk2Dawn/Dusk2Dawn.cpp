/*  Dusk2Dawn.cpp
 *  Get estimate time of sunrise and sunset given a set of coordinates.
 *  Created by DM Kishi <dm.kishi@gmail.com> on 2017-02-01.
 *  Extended with zenith/twilight mode support.
 */

#include "Arduino.h"
#include <math.h>
#include "Dusk2Dawn.h"

Dusk2Dawn::Dusk2Dawn(float latitude, float longitude, float timezone) {
  _latitude  = latitude;
  _longitude = longitude;
  _timezone  = timezone;
}

int Dusk2Dawn::sunrise(int y, int m, int d, bool isDST, float zenith) {
  return sunriseSet(true, y, m, d, isDST, zenith);
}

int Dusk2Dawn::sunset(int y, int m, int d, bool isDST, float zenith) {
  return sunriseSet(false, y, m, d, isDST, zenith);
}

bool Dusk2Dawn::min2str(char *str, int minutes) {
  bool isError = false;
  if (minutes < 0 || minutes >= 1440) { isError = true; }

  float floatHour   = minutes / 60.0;
  float floatMinute = 60.0 * (floatHour - floor(floatHour));
  byte  byteHour    = (byte) floatHour;
  byte  byteMinute  = (byte) floatMinute;

  if (byteMinute > 59) { byteHour += 1; byteMinute = 0; }

  char strHour[]   = "00";
  char strMinute[] = "00";
  isError = isError ? isError : !zeroPadTime(strHour, byteHour);
  isError = isError ? isError : !zeroPadTime(strMinute, byteMinute);

  if (!isError) {
    str[0] = strHour[0]; str[1] = strHour[1]; str[2] = ':';
    str[3] = strMinute[0]; str[4] = strMinute[1]; str[5] = '\0';
  } else {
    str[0]='E'; str[1]='R'; str[2]='R'; str[3]='O'; str[4]='R'; str[5]='\0';
  }
  return !isError;
}

int Dusk2Dawn::sunriseSet(bool isRise, int y, int m, int d, bool isDST, float zenith) {
  float jday, newJday, timeUTC, newTimeUTC;
  int timeLocal;

  jday    = jDay(y, m, d);
  timeUTC = sunriseSetUTC(isRise, jday, _latitude, _longitude, zenith);

  newJday    = jday + timeUTC / (60 * 24);
  newTimeUTC = sunriseSetUTC(isRise, newJday, _latitude, _longitude, zenith);

  if (!isnan(newTimeUTC)) {
    timeLocal  = (int) round(newTimeUTC + (_timezone * 60));
    timeLocal += (isDST) ? 60 : 0;
  } else {
    timeLocal = -1;
  }
  return timeLocal;
}

float Dusk2Dawn::sunriseSetUTC(bool isRise, float jday, float latitude, float longitude, float zenith) {
  float t         = fractionOfCentury(jday);
  float eqTime    = equationOfTime(t);
  float solarDec  = sunDeclination(t);
  float hourAngle = hourAngleSunrise(latitude, solarDec, zenith);

  hourAngle = isRise ? hourAngle : -hourAngle;
  float delta   = longitude + radToDeg(hourAngle);
  float timeUTC = 720 - (4 * delta) - eqTime;
  return timeUTC;
}

float Dusk2Dawn::equationOfTime(float t) {
  float epsilon = obliquityCorrection(t);
  float l0      = geomMeanLongSun(t);
  float e       = eccentricityEarthOrbit(t);
  float m       = geomMeanAnomalySun(t);
  float y       = tan(degToRad(epsilon) / 2); y *= y;
  float sin2l0  = sin(2.0 * degToRad(l0));
  float sinm    = sin(degToRad(m));
  float cos2l0  = cos(2.0 * degToRad(l0));
  float sin4l0  = sin(4.0 * degToRad(l0));
  float sin2m   = sin(2.0 * degToRad(m));
  float Etime   = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0
                  - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
  return radToDeg(Etime) * 4.0;
}

float Dusk2Dawn::meanObliquityOfEcliptic(float t) {
  float seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * 0.001813));
  return 23 + (26 + (seconds / 60)) / 60;
}

float Dusk2Dawn::eccentricityEarthOrbit(float t) {
  return 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
}

float Dusk2Dawn::sunDeclination(float t) {
  float e      = obliquityCorrection(t);
  float lambda = sunApparentLong(t);
  float sint   = sin(degToRad(e)) * sin(degToRad(lambda));
  return radToDeg(asin(sint));
}

float Dusk2Dawn::sunApparentLong(float t) {
  float o      = sunTrueLong(t);
  float omega  = 125.04 - 1934.136 * t;
  return o - 0.00569 - 0.00478 * sin(degToRad(omega));
}

float Dusk2Dawn::sunTrueLong(float t) {
  return geomMeanLongSun(t) + sunEqOfCenter(t);
}

float Dusk2Dawn::sunEqOfCenter(float t) {
  float m     = geomMeanAnomalySun(t);
  float mrad  = degToRad(m);
  float sinm  = sin(mrad);
  float sin2m = sin(mrad * 2);
  float sin3m = sin(mrad * 3);
  return sinm * (1.914602 - t * (0.004817 + 0.000014 * t))
         + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
}

float Dusk2Dawn::hourAngleSunrise(float lat, float solarDec, float zenith) {
  float latRad = degToRad(lat);
  float sdRad  = degToRad(solarDec);
  float HAarg  = (cos(degToRad(zenith)) / (cos(latRad) * cos(sdRad)) - tan(latRad) * tan(sdRad));
  return acos(HAarg);
}

float Dusk2Dawn::obliquityCorrection(float t) {
  float e0    = meanObliquityOfEcliptic(t);
  float omega = 125.04 - 1934.136 * t;
  return e0 + 0.00256 * cos(degToRad(omega));
}

float Dusk2Dawn::geomMeanLongSun(float t) {
  float L0 = 280.46646 + t * (36000.76983 + t * 0.0003032);
  while (L0 > 360) L0 -= 360;
  while (L0 < 0)   L0 += 360;
  return L0;
}

float Dusk2Dawn::geomMeanAnomalySun(float t) {
  return 357.52911 + t * (35999.05029 - 0.0001537 * t);
}

float Dusk2Dawn::jDay(int year, int month, int day) {
  if (month <= 2) { year -= 1; month += 12; }
  int A = floor(year / 100);
  int B = 2 - A + floor(A / 4);
  return floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
}

float Dusk2Dawn::fractionOfCentury(float jd) {
  return (jd - 2451545) / 36525;
}

float Dusk2Dawn::radToDeg(float rad) { return 180 * rad / PI; }
float Dusk2Dawn::degToRad(float deg) { return PI * deg / 180; }

bool Dusk2Dawn::zeroPadTime(char *str, byte timeComponent) {
  if (timeComponent >= 100) return false;
  str[0] = (floor(timeComponent / 10)) + '0';
  str[1] = (timeComponent % 10) + '0';
  return true;
}
