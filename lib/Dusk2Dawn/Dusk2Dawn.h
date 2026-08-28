/*  Dusk2Dawn.h
 *  Get estimate time of sunrise and sunset given a set of coordinates.
 *  Created by DM Kishi <dm.kishi@gmail.com> on 2017-02-01.
 *  Extended with zenith/twilight mode support.
 */

#ifndef Dusk2Dawn_h
#define Dusk2Dawn_h

  #include "Arduino.h"
  #include <math.h>

  // Zenith angles for each twilight mode
  #define ZENITH_REAL        90.833f   // standard sunrise/sunset (refraction + solar disc)
  #define ZENITH_CIVIL       96.0f     // civil twilight
  #define ZENITH_NAUTIC      102.0f    // nautical twilight
  #define ZENITH_ASTRONOMIC  108.0f    // astronomical twilight

  class Dusk2Dawn {
    public:
      Dusk2Dawn(float, float, float);
      int sunrise(int, int, int, bool, float zenith = ZENITH_REAL);
      int sunset(int, int, int, bool, float zenith = ZENITH_REAL);
      static bool min2str(char*, int);
    private:
      float _latitude, _longitude;
      int   _timezone;
      int   sunriseSet(bool, int, int, int, bool, float zenith);
      float sunriseSetUTC(bool, float, float, float, float zenith);
      float equationOfTime(float);
      float meanObliquityOfEcliptic(float);
      float eccentricityEarthOrbit(float);
      float sunDeclination(float);
      float sunApparentLong(float);
      float sunTrueLong(float);
      float sunEqOfCenter(float);
      float hourAngleSunrise(float, float, float zenith);
      float obliquityCorrection(float);
      float geomMeanLongSun(float);
      float geomMeanAnomalySun(float);
      float jDay(int, int, int);
      float fractionOfCentury(float);
      float radToDeg(float);
      float degToRad(float);
      static bool zeroPadTime(char*, byte);
  };

#endif
