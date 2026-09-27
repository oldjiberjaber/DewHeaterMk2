  void Arduino_OTA()
  {
    ArduinoOTA.setHostname(WIFIID);

    ArduinoOTA.onStart([]() {
      isOTAInProgress = true;

      // Shut down all MOSFET heater channels for safety
      for (int i = 0; i < 4; i++) {
        ledcWrite(i, 0);
      }
      tft_setbrightness(200);

      Serial.println(F("\n--- OTA Flashing Started ---"));
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(0, 5);
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.setTextSize(2);
      tft.println(F("OTA UPDATE"));
      
      tft.setTextSize(1);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(0, 30);
      tft.println(F("Receiving firmware..."));

      tft.drawRect(4, 95, 120, 16, ST7735_WHITE);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      if (total == 0) return;
      unsigned int percent = (unsigned int)(((uint64_t)progress * 100) / total);
      static unsigned int last_percent = 999;

      if (percent != last_percent) {
        last_percent = percent;
        Serial.printf("OTA Progress: %u%%\r", percent);

        // Draw Progress Text (only when percent changes)
        tft.setCursor(15, 55);
        tft.setTextSize(3);
        tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
        if (percent < 10) tft.print(F("  "));
        else if (percent < 100) tft.print(F(" "));
        tft.print(percent);
        tft.print(F("%"));

        // Draw Progress Bar
        int barWidth = (percent * 116) / 100;
        if (barWidth > 0) {
          tft.fillRect(6, 97, barWidth, 12, ST7735_CYAN);
        }
      }
    });

    ArduinoOTA.onEnd([]() {
      isOTAInProgress = false;
      Serial.println(F("\nOTA Successful. Rebooting..."));
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 40);
      tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
      tft.setTextSize(2);
      tft.println(F("OTA SUCCESS!"));
      tft.setTextSize(1);
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.setCursor(5, 70);
      tft.println(F("Rebooting device..."));
      
      // Cleanly de-authenticate from WiFi AP so router drops stale lease immediately
      WiFi.disconnect(true, false);
      delay(400);
    });

    ArduinoOTA.onError([](ota_error_t error) {
      isOTAInProgress = false;
      Serial.printf("OTA Error [%u]: ", error);
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(0, 20);
      tft.setTextColor(ST7735_RED, ST7735_BLACK);
      tft.setTextSize(2);
      tft.println(F("OTA FAILED"));
      tft.setTextSize(1);
      if (error == OTA_AUTH_ERROR) { Serial.println(F("Auth Failed")); tft.println(F("Auth Failed")); }
      else if (error == OTA_BEGIN_ERROR) { Serial.println(F("Begin Failed")); tft.println(F("Begin Failed")); }
      else if (error == OTA_CONNECT_ERROR) { Serial.println(F("Connect Failed")); tft.println(F("Connect Failed")); }
      else if (error == OTA_RECEIVE_ERROR) { Serial.println(F("Receive Failed")); tft.println(F("Receive Failed")); }
      else if (error == OTA_END_ERROR) { Serial.println(F("End Failed")); tft.println(F("End Failed")); }
    });

    ArduinoOTA.begin();
  }

  
/*
 * 
 void //////sprinterln(String tobeprinted = "")
{
//Serial.flush();
//Serial.swap();
//Serial.println(tobeprinted); 
//Serial.flush();
//Serial.swap();
}

//void //////sprinter(String tobeprinted = "")
{
//Serial.flush();
//Serial.swap();
// Serial.print(tobeprinted); 
//Serial.flush();
//Serial.swap();
}
*/
