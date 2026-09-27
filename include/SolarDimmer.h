#pragma once
#include <Arduino.h>
#include <time.h>
#include <math.h>

struct SolarInfo {
  bool timeValid;
  float elevationDeg;
  bool isDay;
  int targetBrightness;
};

class SolarDimmer {
public:
  float latitude;
  float longitude;
  int dayBrightness;
  int nightBrightness;
  bool autoDimEnabled;

  SolarDimmer(float lat = 51.5074f, float lon = -0.1278f, int dayBright = 200, int nightBright = 20)
    : latitude(lat), longitude(lon), dayBrightness(dayBright), nightBrightness(nightBright), autoDimEnabled(true) {}

#include <Preferences.h>

  void begin(float defaultLat = 51.5074f, float defaultLon = -0.1278f) {
    Preferences prefs;
    if (prefs.begin("solardim", true)) { // Read-only mode
      latitude = prefs.getFloat("lat", defaultLat);
      longitude = prefs.getFloat("lon", defaultLon);
      autoDimEnabled = prefs.getBool("autodim", true);
      dayBrightness = prefs.getInt("dayb", 200);
      nightBrightness = prefs.getInt("nightb", 20);
      prefs.end();
    } else {
      latitude = defaultLat;
      longitude = defaultLon;
    }
    Serial.printf("SolarDimmer Config: Lat=%.4f, Lon=%.4f, AutoDim=%s\n", 
                  latitude, longitude, autoDimEnabled ? "ON" : "OFF");
  }

  void saveLocation(float lat, float lon) {
    latitude = lat;
    longitude = lon;
    Preferences prefs;
    if (prefs.begin("solardim", false)) { // Read-write mode
      prefs.putFloat("lat", lat);
      prefs.putFloat("lon", lon);
      prefs.end();
      Serial.printf("Saved location to NVS: Lat=%.4f, Lon=%.4f\n", lat, lon);
    }
  }

  void saveAutoDim(bool enabled) {
    autoDimEnabled = enabled;
    Preferences prefs;
    if (prefs.begin("solardim", false)) {
      prefs.putBool("autodim", enabled);
      prefs.end();
    }
  }

  void saveBrightnessLevels(int dayB, int nightB) {
    dayBrightness = constrain(dayB, 0, 255);
    nightBrightness = constrain(nightB, 0, 255);
    Preferences prefs;
    if (prefs.begin("solardim", false)) {
      prefs.putInt("dayb", dayBrightness);
      prefs.putInt("nightb", nightBrightness);
      prefs.end();
    }
  }

  void initNTP(const char* tz = "GMT0BST,M3.5.0/1,M10.5.0", const char* server1 = "pool.ntp.org", const char* server2 = "time.google.com") {
    configTzTime(tz, server1, server2);
    Serial.println(F("NTP Time Sync & Solar Dimmer Initialized"));
  }

  SolarInfo calculateSolar() {
    SolarInfo info;
    info.timeValid = false;
    info.elevationDeg = 0;
    info.isDay = true;
    info.targetBrightness = dayBrightness;

    time_t nowSec = time(nullptr);
    // If timestamp is before 2024 (e.g. initial boot before NTP lock), return default day brightness
    if (nowSec < 1704067200) {
      return info;
    }

    struct tm timeinfo;
    gmtime_r(&nowSec, &timeinfo); // Use UTC for astronomical formulas
    info.timeValid = true;

    // Day of year (1-366)
    int yday = timeinfo.tm_yday + 1;
    double hourUTC = timeinfo.tm_hour + (timeinfo.tm_min / 60.0) + (timeinfo.tm_sec / 3600.0);

    // Fractional year in radians
    double gamma = (2.0 * M_PI / 365.0) * (yday - 1 + ((hourUTC - 12.0) / 24.0));

    // Equation of time in minutes
    double eqtime = 229.18 * (0.000075 + 0.001868 * cos(gamma) - 0.032077 * sin(gamma)
                              - 0.014615 * cos(2.0 * gamma) - 0.040849 * sin(2.0 * gamma));

    // Solar declination angle in radians
    double decl = 0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma)
                  - 0.006758 * cos(2.0 * gamma) + 0.000907 * sin(2.0 * gamma)
                  - 0.002697 * cos(3.0 * gamma) + 0.00148 * sin(3.0 * gamma);

    // True Solar Time in minutes
    double timeOffset = eqtime + (4.0 * longitude);
    double tst = (hourUTC * 60.0) + timeOffset;
    while (tst >= 1440.0) tst -= 1440.0;
    while (tst < 0.0) tst += 1440.0;

    // Solar Hour Angle in radians
    double ha = ((tst / 4.0) - 180.0) * (M_PI / 180.0);

    // Solar Zenith Angle
    double latRad = latitude * (M_PI / 180.0);
    double cosZenith = (sin(latRad) * sin(decl)) + (cos(latRad) * cos(decl) * cos(ha));
    cosZenith = constrain(cosZenith, -1.0, 1.0);
    double zenithRad = acos(cosZenith);
    double elevDeg = 90.0 - (zenithRad * (180.0 / M_PI));

    info.elevationDeg = (float)elevDeg;
    info.isDay = (elevDeg > -0.833); // Astronomical sunrise/sunset horizon

    // Smooth brightness transition:
    // Full day (> +6 deg elevation): dayBrightness (e.g. 200)
    // Full night (<= -6 deg civil twilight): nightBrightness (e.g. 20)
    // Transition (-6 deg to +6 deg): smooth linear interpolation over 12 deg
    if (elevDeg >= 6.0) {
      info.targetBrightness = dayBrightness;
    } else if (elevDeg <= -6.0) {
      info.targetBrightness = nightBrightness;
    } else {
      float factor = ((float)elevDeg + 6.0f) / 12.0f; // 0.0 at -6 deg, 1.0 at +6 deg
      info.targetBrightness = (int)(nightBrightness + factor * (dayBrightness - nightBrightness));
      info.targetBrightness = constrain(info.targetBrightness, nightBrightness, dayBrightness);
    }

    return info;
  }
};

