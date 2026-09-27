
#include <DNSServer.h>
#include <Preferences.h>

String wifi_ssid = "";
String wifi_password = "";
bool isAPMode = false;
DNSServer dnsServer;
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

void loadWiFiFromNVS() {
  Preferences prefs;
  if (prefs.begin("wificfg", true)) {
    wifi_ssid = prefs.getString("ssid", "");
    wifi_password = prefs.getString("pass", "");
    prefs.end();
  }
  // Fallback to AAA_Credentials.h if NVS is empty
  if (wifi_ssid.length() == 0 && String(ssid).length() > 0) {
    wifi_ssid = String(ssid);
    wifi_password = String(password);
  }
}

void saveWiFiToNVS(String newSsid, String newPass) {
  Preferences prefs;
  if (prefs.begin("wificfg", false)) {
    prefs.putString("ssid", newSsid);
    prefs.putString("pass", newPass);
    prefs.putBool("configured", true);
    prefs.end();
    wifi_ssid = newSsid;
    wifi_password = newPass;
    Serial.printf(">>> Saved WiFi Configuration to NVS: SSID='%s'\n", newSsid.c_str());
  }
}

void resetWiFiNVS() {
  Preferences prefs;
  if (prefs.begin("wificfg", false)) {
    prefs.clear();
    prefs.end();
    wifi_ssid = "";
    wifi_password = "";
    Serial.println(">>> Cleared WiFi Configuration from NVS");
  }
}

void startCaptivePortal() {
  isAPMode = true;
  WiFi.disconnect(true, false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  
  String apSSID = "DewHeater-Setup";
  WiFi.softAP(apSSID.c_str());

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  Serial.println(F("\n=========================================="));
  Serial.println(F(">>> Captive Portal / Setup AP Started! <<<"));
  Serial.printf(">>> AP SSID: %s\n", apSSID.c_str());
  Serial.printf(">>> AP IP:   %s\n", apIP.toString().c_str());
  Serial.println(F("==========================================\n"));

  tft_clrscrn();
  tft.println(F("WiFi Setup AP"));
  tft.println(F("-----------------"));
  tft.println(F("Connect to SSID:"));
  tft.println(apSSID);
  tft.println(F(""));
  tft.println(F("Open Browser at:"));
  tft.println(F("192.168.4.1"));
}

void WIFI_connection(void)
{ 
  loadWiFiFromNVS();

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false); // Disable modem sleep for rock-solid connection

  if (wifi_ssid.length() > 0) {
    WiFi.mode(WIFI_STA);
    Serial.println(F(""));
    Serial.print(F("Attempting connection to ")); Serial.println(wifi_ssid);
    tft.println(F("")); tft.println(F("Connecting to:")); tft.println(wifi_ssid);

    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

    unsigned long startMillis = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMillis < 15000)) 
    { 
      delay(500); 
      Serial.print(F("."));
      tft.print(F("."));
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      isAPMode = false;
      Serial.println(F(""));
      Serial.print(F("Connected to ")); Serial.println(wifi_ssid);
      Serial.print(F("IP address: ")); Serial.println(WiFi.localIP());
      Serial.print(F("RSSI: ")); Serial.println(WiFi.RSSI());
      tft.println(F(""));
      tft.println(F("Connected!!"));
      tft.println(F("IP address:")); tft.println(WiFi.localIP());
      tft.print(F("RSSI: ")); tft.println(WiFi.RSSI());

      if (MDNS.begin(WIFIID)) { Serial.println(F("MDNS responder started")); }
      Serial.println(F("WiFi initialization complete"));
      return;
    }

    Serial.println(F("\nWiFi connection timed out! Launching Setup Captive Portal..."));
    tft.println(F("\nTimeout -> AP Mode"));
    delay(1000);
  } else {
    Serial.println(F("\nNo WiFi configured in NVS. Launching Setup Captive Portal..."));
    tft.println(F("\nNo WiFi -> AP Mode"));
    delay(1000);
  }

  startCaptivePortal();
}

int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}
