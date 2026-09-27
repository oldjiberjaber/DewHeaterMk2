#include <QuickPID.h>
#include <math.h>

extern float DP_Offset0;
extern float DP_Offset1;
extern int pwmDuty[];
bool shouldReboot = false;
extern int TFT_Brightness;
extern void tft_setbrightness(int brightness);
extern String Humidity, TempC, Pressure, DP, UV, Rain1h, Rain24h, Pressuretrend;

// access PID tunings from PID.cpp
extern float H0_Kp;
extern float H0_Ki;
extern float H0_Kd;
extern float H1_Kp;
extern float H1_Ki;
extern float H1_Kd;
// access PID controllers to apply new tunings
extern QuickPID heater0;
extern QuickPID heater1;

// Auto-Tuner & Heater Control externs
extern void savePIDToNVS(int heater, float kp, float ki, float kd);
extern bool startTuning(int heater, float stepPWM, uint32_t testSec, uint32_t settleSec, int rule);
extern void abortTuning();
extern void applyTuning(int heater, bool saveToNVS);
extern void discardTuning(int heater);
extern void setHeaterState(int heater, bool enabled);
extern void saveOffsetsToNVS(float off0, float off1);
extern void loadOffsetsFromNVS();
extern TuningStatus tuningStatus;
extern bool heaterEnabled0;
extern bool heaterEnabled1;

// Access PID inputs and setpoints
extern float Input0, Setpoint0;
extern float Input1, Setpoint1;

// Access weather station status
extern bool WXS_Online;
extern bool WeeWX_Valid;

// Access MQTT LWT status
extern bool isLWTOnline;
extern bool setLWTState(bool online);
extern const String mqtt_ID;

// Access Soft-Landing Governor
#ifndef _SOFTLAND_CONFIG_H_
#define _SOFTLAND_CONFIG_H_
struct SoftLandingConfig {
  bool enabled;
  float agg0;
  float agg1;
};
#endif
extern SoftLandingConfig softLandingConfig;
extern float ch0_rate, ch1_rate;
extern bool ch0_landing_active, ch1_landing_active;
extern void saveSoftLandingToNVS(bool enabled, float agg0, float agg1);
extern void loadSoftLandingFromNVS();

// Access Adaptive Learning Engine
#ifndef _ADAPTIVE_LEARNING_H_
#define _ADAPTIVE_LEARNING_H_
struct AdaptiveLearningConfig {
  bool enabled;
  int adaptationSpeed;
  float kloss0;
  float kloss1;
  float leadSec0;
  float leadSec1;
  int cycles0;
  int cycles1;
  float lastError0;
  float lastError1;
  float confidence0;
  float confidence1;
};
#endif
extern AdaptiveLearningConfig learningConfig;
extern int engineState0, engineState1;
extern void saveLearningToNVS();
extern void resetLearningToDefaults();

// Access WiFi Config & AP Mode
extern String wifi_ssid, wifi_password;
extern bool isAPMode;
extern void saveWiFiToNVS(String newSsid, String newPass);
extern void resetWiFiNVS();
extern int WifiGetRssiAsQuality(int rssi);

const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>DewHeater Mk2 | WiFi Setup</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 30, 49, 0.9);
      --border: rgba(255, 255, 255, 0.12);
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --primary: #0284c7;
      --accent: #38bdf8;
      --success: #34d399;
      --danger: #ef4444;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
      background: radial-gradient(circle at 50% 20%, #1e293b 0%, #090d16 100%);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 16px;
    }
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      border: 1px solid var(--border);
      border-radius: 16px;
      max-width: 440px;
      width: 100%;
      padding: 28px 24px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.5);
    }
    .header { text-align: center; margin-bottom: 24px; }
    .header h1 { font-size: 1.45rem; font-weight: 700; color: var(--text); margin-bottom: 6px; }
    .header p { font-size: 0.88rem; color: var(--text-muted); }
    .form-group { margin-bottom: 18px; }
    label { display: block; font-size: 0.82rem; font-weight: 600; color: var(--text-muted); margin-bottom: 6px; }
    select, input[type="text"], input[type="password"] {
      width: 100%;
      background: rgba(15, 23, 42, 0.85);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 12px 14px;
      color: var(--text);
      font-size: 0.95rem;
      outline: none;
      transition: border-color 0.2s;
    }
    select:focus, input:focus { border-color: var(--accent); }
    .pass-wrap { position: relative; }
    .btn-toggle-pass {
      position: absolute;
      right: 12px;
      top: 50%;
      transform: translateY(-50%);
      background: none;
      border: none;
      cursor: pointer;
      font-size: 1.1rem;
      color: var(--text-muted);
    }
    .btn {
      display: block;
      width: 100%;
      padding: 12px 16px;
      border-radius: 8px;
      font-size: 0.95rem;
      font-weight: 600;
      border: none;
      cursor: pointer;
      transition: all 0.2s ease;
      text-align: center;
    }
    .btn-primary { background: linear-gradient(135deg, #0284c7, #0ea5e9); color: #fff; margin-top: 8px; }
    .btn-primary:hover { opacity: 0.9; }
    .btn-secondary { background: rgba(255,255,255,0.08); color: var(--text); margin-top: 10px; }
    .btn-secondary:hover { background: rgba(255,255,255,0.14); }
    .status-msg { margin-top: 14px; font-size: 0.88rem; text-align: center; display: none; line-height: 1.4; }
    .spinner { display: inline-block; width: 14px; height: 14px; border: 2px solid rgba(255,255,255,0.3); border-radius: 50%; border-top-color: #fff; animation: spin 0.8s linear infinite; vertical-align: middle; margin-right: 6px; }
    @keyframes spin { to { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div class="card">
    <div class="header">
      <h1>🔭 DewHeater Mk2</h1>
      <p>Observatory Controller WiFi Setup</p>
    </div>

    <div class="form-group">
      <label for="wifi-select">Nearby 2.4GHz Networks</label>
      <select id="wifi-select" onchange="onSelectWifi(this.value)">
        <option value="">Scanning networks...</option>
      </select>
    </div>

    <div class="form-group">
      <label for="wifi-ssid">Network Name (SSID)</label>
      <input type="text" id="wifi-ssid" placeholder="Enter or select WiFi name" autocomplete="off">
    </div>

    <div class="form-group">
      <label for="wifi-pass">WiFi Password</label>
      <div class="pass-wrap">
        <input type="password" id="wifi-pass" placeholder="Enter WiFi password" autocomplete="off">
        <button type="button" class="btn-toggle-pass" onclick="togglePass()" title="Show/Hide Password">👁️</button>
      </div>
    </div>

    <button type="button" class="btn btn-primary" id="btn-save" onclick="saveAndConnect()">💾 Save & Connect</button>
    <button type="button" class="btn btn-secondary" onclick="scanNetworks()">🔄 Rescan Networks</button>

    <div id="status-box" class="status-msg"></div>
  </div>

  <script>
    function togglePass() {
      const p = document.getElementById('wifi-pass');
      p.type = p.type === 'password' ? 'text' : 'password';
    }

    function onSelectWifi(val) {
      if (val) {
        document.getElementById('wifi-ssid').value = val;
        document.getElementById('wifi-pass').focus();
      }
    }

    function scanNetworks() {
      const sel = document.getElementById('wifi-select');
      sel.innerHTML = '<option value="">Scanning networks...</option>';
      fetch('/api/wifi/scan')
        .then(r => r.json())
        .then(networks => {
          sel.innerHTML = '<option value="">-- Select a detected network --</option>';
          if (!networks || networks.length === 0) {
            sel.innerHTML += '<option value="" disabled>No networks found (Enter manually below)</option>';
            return;
          }
          networks.forEach(net => {
            const opt = document.createElement('option');
            opt.value = net.ssid;
            const lockIcon = net.secure ? '🔒' : '🔓';
            opt.textContent = `${lockIcon} ${net.ssid} (${net.rssi} dBm, ${net.quality}%)`;
            sel.appendChild(opt);
          });
        })
        .catch(err => {
          sel.innerHTML = '<option value="">Scan failed (Enter SSID manually)</option>';
        });
    }

    function saveAndConnect() {
      const ssid = document.getElementById('wifi-ssid').value.trim();
      const pass = document.getElementById('wifi-pass').value;
      const statusBox = document.getElementById('status-box');
      const btn = document.getElementById('btn-save');

      if (!ssid) {
        alert('Please enter or select an SSID network name.');
        return;
      }

      btn.disabled = true;
      btn.innerHTML = '<span class="spinner"></span> Saving & Connecting...';
      statusBox.style.display = 'block';
      statusBox.style.color = '#38bdf8';
      statusBox.innerHTML = 'Connecting to <strong>' + ssid + '</strong>... The controller will restart and join your network.';

      fetch('/api/wifi/connect?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
        .then(r => r.json())
        .then(data => {
          statusBox.style.color = '#34d399';
          statusBox.innerHTML = '✅ Saved! Connecting now... Check your router or TFT display for the new IP address.';
        })
        .catch(() => {
          statusBox.style.color = '#34d399';
          statusBox.innerHTML = '✅ Saved! Rebooting to connect to ' + ssid + '...';
        });
    }

    window.onload = function() {
      scanNetworks();
    };
  </script>
</body>
</html>
)rawliteral";

#include "SolarDimmer.h"
extern SolarDimmer solarDimmer;

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  String jsonString = "";
  if (acquireDataMutex(pdMS_TO_TICKS(100))) {
    readings["out_temp"] = TempC.toFloat();
    readings["humidity"] =  Humidity.toFloat();
    readings["dew_point"] = DP.toFloat();
    readings["wxs_online"] = WXS_Online;
    readings["weewx_valid"] = WeeWX_Valid;
    readings["lwt_online"] = isLWTOnline;
    readings["softland_enabled"] = softLandingConfig.enabled;
    readings["softland_agg0"] = softLandingConfig.agg0;
    readings["softland_agg1"] = softLandingConfig.agg1;
    readings["ch0_rate"] = ch0_rate;
    readings["ch1_rate"] = ch1_rate;
    readings["ch0_landing_active"] = ch0_landing_active;
    readings["ch1_landing_active"] = ch1_landing_active;
    readings["learning_enabled"] = learningConfig.enabled;
    readings["learning_speed"] = learningConfig.adaptationSpeed;
    readings["kloss0"] = learningConfig.kloss0;
    readings["kloss1"] = learningConfig.kloss1;
    readings["lead0"] = learningConfig.leadSec0;
    readings["lead1"] = learningConfig.leadSec1;
    readings["cycles0"] = learningConfig.cycles0;
    readings["cycles1"] = learningConfig.cycles1;
    readings["lasterr0"] = learningConfig.lastError0;
    readings["lasterr1"] = learningConfig.lastError1;
    readings["conf0"] = learningConfig.confidence0;
    readings["conf1"] = learningConfig.confidence1;
    readings["eng_state0"] = engineState0;
    readings["eng_state1"] = engineState1;
    readings["pwr1"] = returnPWR(ledcRead(0));
    readings["temp1"] = DT[0].Temperature;
    readings["input0"] = Input0;
    readings["setpoint0"] = Setpoint0;
    readings["offset1"] = DP_Offset0;
    readings["heater0_enabled"] = heaterEnabled0;
    readings["pwr2"] = returnPWR(ledcRead(1));
    readings["temp2"] = DT[1].Temperature;
    readings["input1"] = Input1;
    readings["setpoint1"] = Setpoint1;
    readings["offset2"] = DP_Offset1;
    readings["heater1_enabled"] = heaterEnabled1;
    readings["brightness"] = TFT_Brightness;
    readings["rssi"] = WiFi.RSSI();
    readings["wifi_quality"] = WifiGetRssiAsQuality(WiFi.RSSI());
    readings["wifi_ssid"] = isAPMode ? "DewHeater-Setup" : wifi_ssid;
    readings["is_ap_mode"] = isAPMode;
    readings["ip_addr"] = isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    
    SolarInfo sInfo = solarDimmer.calculateSolar();
    readings["auto_dim"] = solarDimmer.autoDimEnabled;
    readings["solar_elev"] = sInfo.timeValid ? sInfo.elevationDeg : 0;
    readings["is_day"] = sInfo.isDay;

    serializeJson(readings, jsonString);
    releaseDataMutex();
  }
  return jsonString;
}

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initWebServer() {   
  initLittleFS();

  // Captive Portal Detection Redirects (Apple, Android, Windows)
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
  server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
    request->redirect("http://192.168.4.1/");
  });

  // Diagnostic endpoint to inspect LittleFS contents
  server.on("/api/fs", HTTP_GET, [](AsyncWebServerRequest *request){
    String output = "LittleFS Files:\n";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
      output += String(file.name()) + " (" + String(file.size()) + " bytes)\n";
      file = root.openNextFile();
    }
    request->send(200, "text/plain", output);
  });

  // Web Server Root URL - Serves Captive Portal setup in AP mode, Dashboard in Station mode
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (isAPMode) {
      request->send(200, "text/html", CAPTIVE_PORTAL_HTML);
    } else {
      request->send(LittleFS, "/index.html", "text/html");
    }
  });

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });


  // Endpoint to set offsets: /set_offset?out=1&val=5.5
  server.on("/set_offset", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("out") && request->hasArg("val")) {
      int out = request->arg("out").toInt();
      float val = request->arg("val").toFloat();
      if (out == 1) DP_Offset0 = val;
      else if (out == 2) DP_Offset1 = val;
      request->send(200, "text/plain", "OK");
      StaticJsonDocument<128> doc;
      doc["offset1"] = DP_Offset0;
      doc["offset2"] = DP_Offset1;
      String s; serializeJson(doc, s);
      events.send(s.c_str(), "offsets", millis());
    } else {
      request->send(400, "text/plain", "missing parameters");
    }
  });

  // Also handle trailing-slash variant to prevent LittleFS directory lookups
  server.on("/set_offset/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("out") && request->hasArg("val")) {
      int out = request->arg("out").toInt();
      float val = request->arg("val").toFloat();
      if (out == 1) DP_Offset0 = val;
      else if (out == 2) DP_Offset1 = val;
      request->send(200, "text/plain", "OK");
      StaticJsonDocument<128> doc;
      doc["offset1"] = DP_Offset0;
      doc["offset2"] = DP_Offset1;
      String s; serializeJson(doc, s);
      events.send(s.c_str(), "offsets", millis());
    } else {
      request->send(200, "text/plain", "Usage: /set_offset?out=<1|2>&val=<offset>");
    }
  });

  // Endpoint to set PWM duty cycle (0-100%): /set_pwm?out=1&val=50
  server.on("/set_pwm", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("out") && request->hasArg("val")) {
      int out = request->arg("out").toInt();
      int val = request->arg("val").toInt();
      if (out >= 1 && out <= numChannels) {
        pwmDuty[out-1] = constrain(val, 0, 100);
        // apply immediately
        ledcWrite(out-1, map(pwmDuty[out-1], 0, 100, 0, 255));
        request->send(200, "text/plain", "OK");
        StaticJsonDocument<128> doc;
        doc["pwm1"] = pwmDuty[0];
        doc["pwm2"] = pwmDuty[1];
        String s; serializeJson(doc, s);
        events.send(s.c_str(), "pwm_update", millis());
      } else {
        request->send(400, "text/plain", "invalid output");
      }
    } else {
      request->send(400, "text/plain", "missing parameters");
    }
  });

  // Also handle trailing-slash variant to prevent LittleFS from trying to serve a directory index
  server.on("/set_pwm/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("out") && request->hasArg("val")) {
      int out = request->arg("out").toInt();
      int val = request->arg("val").toInt();
      if (out >= 1 && out <= numChannels) {
        pwmDuty[out-1] = constrain(val, 0, 100);
        // apply immediately
        ledcWrite(out-1, map(pwmDuty[out-1], 0, 100, 0, 255));
        request->send(200, "text/plain", "OK");
        StaticJsonDocument<128> doc;
        doc["pwm1"] = pwmDuty[0];
        doc["pwm2"] = pwmDuty[1];
        String s; serializeJson(doc, s);
        events.send(s.c_str(), "pwm_update", millis());
      } else {
        request->send(400, "text/plain", "invalid output");
      }
    } else {
      // No args provided; respond with usage message to avoid directory lookup
      request->send(200, "text/plain", "Usage: /set_pwm?out=<1..4>&val=<0..100>");
    }
  });

  // Endpoint to control heater state (enable or cooldown off): /set_heater?heater=<0|1|2>&state=<1|0> (2=All)
  server.on("/set_heater", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("heater") && request->hasArg("state")) {
      int heater = request->arg("heater").toInt();
      bool state = (request->arg("state").toInt() > 0);
      setHeaterState(heater, state);
      StaticJsonDocument<128> doc;
      doc["status"] = "OK";
      doc["heater0_enabled"] = heaterEnabled0;
      doc["heater1_enabled"] = heaterEnabled1;
      String s; serializeJson(doc, s);
      request->send(200, "application/json", s);
      events.send(s.c_str(), "heater_state", millis());
    } else {
      request->send(400, "text/plain", "Usage: /set_heater?heater=<0|1|2>&state=<1|0>");
    }
  });

  // Endpoint to set TFT Brightness (0-255): /set_brightness?val=200
  server.on("/set_brightness", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("val")) {
      int val = request->arg("val").toInt();
      val = constrain(val, 0, 255);
      TFT_Brightness = val;
      if (request->hasArg("auto")) {
        solarDimmer.autoDimEnabled = (request->arg("auto").toInt() > 0);
      }
      tft_setbrightness(val);
      request->send(200, "text/plain", "OK");
      StaticJsonDocument<64> doc;
      doc["brightness"] = val;
      doc["auto_dim"] = solarDimmer.autoDimEnabled;
      String s; serializeJson(doc, s);
      events.send(s.c_str(), "brightness_update", millis());
    } else {
      request->send(400, "text/plain", "missing val");
    }
  });

  // Endpoint to toggle Auto Dimming: /set_autodim?val=1|0
  server.on("/set_autodim", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("val")) {
      bool enabled = (request->arg("val").toInt() > 0);
      solarDimmer.saveAutoDim(enabled);
      request->send(200, "text/plain", "OK");
      StaticJsonDocument<64> doc;
      doc["auto_dim"] = solarDimmer.autoDimEnabled;
      String s; serializeJson(doc, s);
      events.send(s.c_str(), "brightness_update", millis());
    } else {
      request->send(400, "text/plain", "missing val");
    }
  });

  // Endpoint to set Coordinates: /set_location?lat=51.5&lon=-0.12
  server.on("/set_location", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("lat") && request->hasArg("lon")) {
      float lat = request->arg("lat").toFloat();
      float lon = request->arg("lon").toFloat();
      solarDimmer.saveLocation(lat, lon);
      StaticJsonDocument<128> doc;
      doc["status"] = "OK";
      doc["lat"] = solarDimmer.latitude;
      doc["lon"] = solarDimmer.longitude;
      String s; serializeJson(doc, s);
      request->send(200, "application/json", s);
    } else {
      request->send(400, "text/plain", "missing lat/lon");
    }
  });

  // Endpoint to set Day and Night Dimming levels: /set_dim_levels?day=200&night=20
  server.on("/set_dim_levels", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("day") && request->hasArg("night")) {
      int dayB = request->arg("day").toInt();
      int nightB = request->arg("night").toInt();
      solarDimmer.saveBrightnessLevels(dayB, nightB);
      if (solarDimmer.autoDimEnabled) {
        SolarInfo sInfo = solarDimmer.calculateSolar();
        tft_setbrightness(sInfo.targetBrightness);
      }
      StaticJsonDocument<128> doc;
      doc["status"] = "OK";
      doc["day_bright"] = solarDimmer.dayBrightness;
      doc["night_bright"] = solarDimmer.nightBrightness;
      String s; serializeJson(doc, s);
      request->send(200, "application/json", s);
      events.send(s.c_str(), "dim_levels_update", millis());
    } else {
      request->send(400, "text/plain", "missing day/night params");
    }
  });


  // Endpoint to get location and solar status: /api/location
  server.on("/api/location", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<256> doc;
    doc["lat"] = solarDimmer.latitude;
    doc["lon"] = solarDimmer.longitude;
    doc["auto_dim"] = solarDimmer.autoDimEnabled;
    doc["day_bright"] = solarDimmer.dayBrightness;
    doc["night_bright"] = solarDimmer.nightBrightness;
    SolarInfo sInfo = solarDimmer.calculateSolar();
    doc["solar_elev"] = sInfo.timeValid ? sInfo.elevationDeg : 0;
    doc["is_day"] = sInfo.isDay;
    doc["time_valid"] = sInfo.timeValid;
    String s; serializeJson(doc, s);
    request->send(200, "application/json", s);
  });

  // Build information endpoint (use snprintf to avoid allocation issues)
  server.on("/api/buildinfo", HTTP_GET, [](AsyncWebServerRequest *request){
    char buf[256];
    snprintf(buf, sizeof(buf), "{\"version\":\"%s\",\"date\":\"%s\",\"time\":\"%s\"}", SW_VERSION, COMPILE_DATE, COMPILE_TIME);
    request->send(200, "application/json", String(buf));
  });
  
  // PID get endpoint - returns current tunings for both heaters
  server.on("/api/pid", HTTP_GET, [](AsyncWebServerRequest *request){
    float h0kp = (isnan(H0_Kp) || isinf(H0_Kp) || H0_Kp <= 0) ? 25.0f : H0_Kp;
    float h0ki = (isnan(H0_Ki) || isinf(H0_Ki) || H0_Ki < 0) ? 10.0f : H0_Ki;
    float h0kd = (isnan(H0_Kd) || isinf(H0_Kd) || H0_Kd < 0) ? 25.0f : H0_Kd;
    float h1kp = (isnan(H1_Kp) || isinf(H1_Kp) || H1_Kp <= 0) ? 25.0f : H1_Kp;
    float h1ki = (isnan(H1_Ki) || isinf(H1_Ki) || H1_Ki < 0) ? 4.0f : H1_Ki;
    float h1kd = (isnan(H1_Kd) || isinf(H1_Kd) || H1_Kd < 0) ? 10.0f : H1_Kd;

    char buf[256];
    snprintf(buf, sizeof(buf), "{\"H0\":{\"Kp\":%.3f,\"Ki\":%.3f,\"Kd\":%.3f},\"H1\":{\"Kp\":%.3f,\"Ki\":%.3f,\"Kd\":%.3f}}",
             h0kp, h0ki, h0kd, h1kp, h1ki, h1kd);
    request->send(200, "application/json", String(buf));
  });
  
  // PID set endpoint - usage: /set_pid?heater=0&kp=..&ki=..&kd=..
  server.on("/set_pid", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasArg("heater")) {
      request->send(400, "text/plain", "missing heater param");
      return;
    }
    int heater = request->arg("heater").toInt();
    float kp = request->hasArg("kp") ? request->arg("kp").toFloat() : NAN;
    float ki = request->hasArg("ki") ? request->arg("ki").toFloat() : NAN;
    float kd = request->hasArg("kd") ? request->arg("kd").toFloat() : NAN;

    if (heater == 0) {
      if (!isnan(kp)) H0_Kp = kp;
      if (!isnan(ki)) H0_Ki = ki;
      if (!isnan(kd)) H0_Kd = kd;
      heater0.SetTunings(H0_Kp, H0_Ki, H0_Kd);
      savePIDToNVS(0, H0_Kp, H0_Ki, H0_Kd);
    } else if (heater == 1) {
      if (!isnan(kp)) H1_Kp = kp;
      if (!isnan(ki)) H1_Ki = ki;
      if (!isnan(kd)) H1_Kd = kd;
      heater1.SetTunings(H1_Kp, H1_Ki, H1_Kd);
      savePIDToNVS(1, H1_Kp, H1_Ki, H1_Kd);
    } else {
      request->send(400, "text/plain", "invalid heater");
      return;
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"status\":\"OK\",\"heater\":%d}", heater);
    request->send(200, "application/json", String(resp));
  });

  // Endpoint to get default offsets: /api/offsets
  server.on("/api/offsets", HTTP_GET, [](AsyncWebServerRequest *request){
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"offset0\":%.2f,\"offset1\":%.2f}", DP_Offset0, DP_Offset1);
    request->send(200, "application/json", String(buf));
  });

  // Endpoint to set and persist default offsets: /set_default_offsets?offset0=4.5&offset1=3.0
  server.on("/set_default_offsets", HTTP_GET, [](AsyncWebServerRequest *request){
    float off0 = DP_Offset0;
    float off1 = DP_Offset1;
    if (request->hasArg("offset0")) off0 = request->arg("offset0").toFloat();
    if (request->hasArg("offset1")) off1 = request->arg("offset1").toFloat();
    saveOffsetsToNVS(off0, off1);

    StaticJsonDocument<128> doc;
    doc["status"] = "OK";
    doc["offset0"] = DP_Offset0;
    doc["offset1"] = DP_Offset1;
    String s; serializeJson(doc, s);
    request->send(200, "application/json", s);

    StaticJsonDocument<128> offDoc;
    offDoc["offset1"] = DP_Offset0;
    offDoc["offset2"] = DP_Offset1;
    String offStr; serializeJson(offDoc, offStr);
    events.send(offStr.c_str(), "offsets", millis());
  });

  // --- PID AUTO-TUNER ENDPOINTS ---
  // Tuning Status endpoint
  server.on("/api/tuning/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<512> doc;
    doc["state"] = (int)tuningStatus.state;
    doc["heater"] = tuningStatus.heater;
    doc["temp"] = tuningStatus.currentTemp;
    doc["output"] = tuningStatus.currentOutput;
    
    unsigned long elapsed = 0;
    if (tuningStatus.state == 1 || tuningStatus.state == 2) {
      elapsed = (millis() - tuningStatus.startMillis) / 1000;
    }
    uint32_t totalSec = tuningStatus.settleTime + tuningStatus.testTime;
    doc["elapsed"] = elapsed;
    doc["total"] = totalSec;
    doc["progress"] = (totalSec > 0 && elapsed > 0) ? min(100.0f, (float)elapsed * 100.0f / totalSec) : (tuningStatus.state == 3 ? 100.0f : 0.0f);
    
    doc["orig_kp"] = tuningStatus.orig_Kp;
    doc["orig_ki"] = tuningStatus.orig_Ki;
    doc["orig_kd"] = tuningStatus.orig_Kd;

    doc["calc_kp"] = tuningStatus.calc_Kp;
    doc["calc_ki"] = tuningStatus.calc_Ki;
    doc["calc_kd"] = tuningStatus.calc_Kd;
    doc["gain"] = tuningStatus.processGain;
    doc["deadtime"] = tuningStatus.deadTime;
    doc["tau"] = tuningStatus.tau;
    doc["msg"] = tuningStatus.message;

    String resp;
    serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // Start Tuning: /api/tuning/start?heater=0&step=128&testTime=60&settleTime=10&rule=0
  server.on("/api/tuning/start", HTTP_GET, [](AsyncWebServerRequest *request){
    int heater = request->hasArg("heater") ? request->arg("heater").toInt() : 0;
    float step = request->hasArg("step") ? request->arg("step").toFloat() : 204.0f;
    uint32_t testTime = request->hasArg("testTime") ? request->arg("testTime").toInt() : (heater == 1 ? 240 : 120);
    uint32_t settleTime = request->hasArg("settleTime") ? request->arg("settleTime").toInt() : 20;
    int rule = request->hasArg("rule") ? request->arg("rule").toInt() : 0;

    bool ok = startTuning(heater, step, testTime, settleTime, rule);
    StaticJsonDocument<128> doc;
    doc["status"] = ok ? "OK" : "ERROR";
    doc["msg"] = tuningStatus.message;
    String resp; serializeJson(doc, resp);
    request->send(ok ? 200 : 400, "application/json", resp);
  });

  // Stop / Abort Tuning
  server.on("/api/tuning/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    abortTuning();
    StaticJsonDocument<128> doc;
    doc["status"] = "OK";
    doc["msg"] = "Tuning aborted";
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // Apply Calculated Tunings: /api/tuning/apply?heater=0&save=1
  server.on("/api/tuning/apply", HTTP_GET, [](AsyncWebServerRequest *request){
    int heater = request->hasArg("heater") ? request->arg("heater").toInt() : tuningStatus.heater;
    bool saveNVS = request->hasArg("save") ? (request->arg("save").toInt() > 0) : true;
    applyTuning(heater, saveNVS);
    StaticJsonDocument<128> doc;
    doc["status"] = "OK";
    doc["msg"] = tuningStatus.message;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // Discard Calculated Tunings: /api/tuning/discard?heater=0
  server.on("/api/tuning/discard", HTTP_GET, [](AsyncWebServerRequest *request){
    int heater = request->hasArg("heater") ? request->arg("heater").toInt() : tuningStatus.heater;
    discardTuning(heater);
    StaticJsonDocument<128> doc;
    doc["status"] = "OK";
    doc["msg"] = tuningStatus.message;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // LWT Automation Testing Endpoints
  server.on("/api/lwt/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
    bool nextState = !isLWTOnline;
    bool ok = setLWTState(nextState);
    StaticJsonDocument<128> doc;
    doc["status"] = ok ? "OK" : "ERROR";
    doc["lwt_online"] = isLWTOnline;
    doc["lwt_state"] = isLWTOnline ? "Online" : "Offline";
    doc["topic"] = "tele/" + mqtt_ID + "/LWT";
    String resp; serializeJson(doc, resp);
    request->send(ok ? 200 : 500, "application/json", resp);
  });

  server.on("/api/lwt/set", HTTP_GET, [](AsyncWebServerRequest *request){
    bool state = true;
    if (request->hasArg("state")) {
      String s = request->arg("state");
      s.toLowerCase();
      if (s == "offline" || s == "0" || s == "false") state = false;
    }
    bool ok = setLWTState(state);
    StaticJsonDocument<128> doc;
    doc["status"] = ok ? "OK" : "ERROR";
    doc["lwt_online"] = isLWTOnline;
    doc["lwt_state"] = isLWTOnline ? "Online" : "Offline";
    doc["topic"] = "tele/" + mqtt_ID + "/LWT";
    String resp; serializeJson(doc, resp);
    request->send(ok ? 200 : 500, "application/json", resp);
  });

  // Soft-Landing Governor Endpoints
  server.on("/api/softland/set", HTTP_GET, [](AsyncWebServerRequest *request){
    bool enabled = softLandingConfig.enabled;
    float agg0 = softLandingConfig.agg0;
    float agg1 = softLandingConfig.agg1;

    if (request->hasArg("enabled")) {
      String en = request->arg("enabled");
      enabled = (en == "1" || en == "true" || en == "TRUE");
    }
    if (request->hasArg("agg0")) {
      agg0 = request->arg("agg0").toFloat();
    }
    if (request->hasArg("agg1")) {
      agg1 = request->arg("agg1").toFloat();
    }

    saveSoftLandingToNVS(enabled, agg0, agg1);

    StaticJsonDocument<256> doc;
    doc["status"] = "OK";
    doc["enabled"] = softLandingConfig.enabled;
    doc["agg0"] = softLandingConfig.agg0;
    doc["agg1"] = softLandingConfig.agg1;
    doc["ch0_rate"] = ch0_rate;
    doc["ch1_rate"] = ch1_rate;
    doc["ch0_landing_active"] = ch0_landing_active;
    doc["ch1_landing_active"] = ch1_landing_active;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/api/softland", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("enabled") || request->hasArg("agg0") || request->hasArg("agg1")) {
      bool enabled = softLandingConfig.enabled;
      float agg0 = softLandingConfig.agg0;
      float agg1 = softLandingConfig.agg1;

      if (request->hasArg("enabled")) {
        String en = request->arg("enabled");
        enabled = (en == "1" || en == "true" || en == "TRUE");
      }
      if (request->hasArg("agg0")) {
        agg0 = request->arg("agg0").toFloat();
      }
      if (request->hasArg("agg1")) {
        agg1 = request->arg("agg1").toFloat();
      }
      saveSoftLandingToNVS(enabled, agg0, agg1);
    }

    StaticJsonDocument<256> doc;
    doc["status"] = "OK";
    doc["enabled"] = softLandingConfig.enabled;
    doc["agg0"] = softLandingConfig.agg0;
    doc["agg1"] = softLandingConfig.agg1;
    doc["ch0_rate"] = ch0_rate;
    doc["ch1_rate"] = ch1_rate;
    doc["ch0_landing_active"] = ch0_landing_active;
    doc["ch1_landing_active"] = ch1_landing_active;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // --- Adaptive Self-Learning Thermal Engine Endpoints ---
  server.on("/api/learning/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    resetLearningToDefaults();
    StaticJsonDocument<256> doc;
    doc["status"] = "OK";
    doc["msg"] = "Learned thermal model reset to factory defaults";
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/api/learning/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("enabled")) {
      String en = request->arg("enabled");
      learningConfig.enabled = (en == "1" || en == "true" || en == "TRUE");
    }
    if (request->hasArg("speed")) {
      learningConfig.adaptationSpeed = constrain(request->arg("speed").toInt(), 0, 2);
    }
    saveLearningToNVS();

    StaticJsonDocument<256> doc;
    doc["status"] = "OK";
    doc["enabled"] = learningConfig.enabled;
    doc["speed"] = learningConfig.adaptationSpeed;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/api/learning", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("enabled") || request->hasArg("speed")) {
      if (request->hasArg("enabled")) {
        String en = request->arg("enabled");
        learningConfig.enabled = (en == "1" || en == "true" || en == "TRUE");
      }
      if (request->hasArg("speed")) {
        learningConfig.adaptationSpeed = constrain(request->arg("speed").toInt(), 0, 2);
      }
      saveLearningToNVS();
    }

    StaticJsonDocument<512> doc;
    doc["status"] = "OK";
    doc["enabled"] = learningConfig.enabled;
    doc["speed"] = learningConfig.adaptationSpeed;
    doc["kloss0"] = learningConfig.kloss0;
    doc["kloss1"] = learningConfig.kloss1;
    doc["lead0"] = learningConfig.leadSec0;
    doc["lead1"] = learningConfig.leadSec1;
    doc["cycles0"] = learningConfig.cycles0;
    doc["cycles1"] = learningConfig.cycles1;
    doc["lasterr0"] = learningConfig.lastError0;
    doc["lasterr1"] = learningConfig.lastError1;
    doc["conf0"] = learningConfig.confidence0;
    doc["conf1"] = learningConfig.confidence1;
    doc["eng_state0"] = engineState0;
    doc["eng_state1"] = engineState1;
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  // --- WiFi & Captive Portal Management Endpoints ---
  server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<256> doc;
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ap_mode"] = isAPMode;
    doc["ssid"] = isAPMode ? "DewHeater-Setup" : wifi_ssid;
    doc["ip"] = isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["quality"] = WifiGetRssiAsQuality(WiFi.RSSI());
    doc["mac"] = WiFi.macAddress();
    doc["gateway"] = isAPMode ? "192.168.4.1" : WiFi.gatewayIP().toString();
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    int n = WiFi.scanNetworks(false, true);
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();
    if (n > 0) {
      for (int i = 0; i < n && i < 15; ++i) {
        String s = WiFi.SSID(i);
        if (s.length() == 0) continue;
        // Deduplicate
        bool duplicate = false;
        for (JsonObject item : arr) {
          if (item["ssid"].as<String>() == s) {
            duplicate = true;
            break;
          }
        }
        if (duplicate) continue;
        JsonObject net = arr.createNestedObject();
        net["ssid"] = s;
        net["rssi"] = WiFi.RSSI(i);
        net["quality"] = WifiGetRssiAsQuality(WiFi.RSSI(i));
        net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
    }
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/api/wifi/save", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (request->hasArg("ssid")) {
      String newSsid = request->arg("ssid");
      String newPass = request->hasArg("pass") ? request->arg("pass") : "";
      saveWiFiToNVS(newSsid, newPass);
      if (request->hasArg("reboot") && (request->arg("reboot") == "1" || request->arg("reboot") == "true")) {
        shouldReboot = true;
      }
      StaticJsonDocument<128> doc;
      doc["status"] = "OK";
      doc["ssid"] = newSsid;
      String resp; serializeJson(doc, resp);
      request->send(200, "application/json", resp);
    } else {
      request->send(400, "text/plain", "missing ssid param");
    }
  });

  server.on("/api/wifi/connect", HTTP_ANY, [](AsyncWebServerRequest *request){
    if (request->hasArg("ssid")) {
      String newSsid = request->arg("ssid");
      String newPass = request->hasArg("pass") ? request->arg("pass") : "";
      saveWiFiToNVS(newSsid, newPass);
      shouldReboot = true;
      StaticJsonDocument<128> doc;
      doc["status"] = "OK";
      doc["msg"] = "Saved credentials. Rebooting to join " + newSsid;
      String resp; serializeJson(doc, resp);
      request->send(200, "application/json", resp);
    } else {
      request->send(400, "text/plain", "missing ssid param");
    }
  });

  server.on("/api/wifi/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    resetWiFiNVS();
    shouldReboot = true;
    StaticJsonDocument<128> doc;
    doc["status"] = "OK";
    doc["msg"] = "WiFi credentials cleared. Rebooting to Setup AP mode";
    String resp; serializeJson(doc, resp);
    request->send(200, "application/json", resp);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Resetting...");
    shouldReboot = true;
  });

  // Handle favicon requests without filesystem lookup
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204);
  });
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(204);
  });

  // Serve static files (CSS, JS, images) from LittleFS as fallback after API routes
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  events.onConnect([](AsyncEventSourceClient *client){

    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
}
