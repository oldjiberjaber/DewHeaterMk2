void init_tft() {
#ifdef USE_TFT_DISPLAY
  tft.initR(INITR_144GREENTAB); // initialise ST7735S chip, green tab
  tft.setTextWrap(false); // Allow text to run off right edge
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1); 
  //set pin TFT_LED as a PWM pin, assign it to channel 10, 5kHz and 8bit resolution
  ledcSetup(TFT_PWM_Channel, 5000, 8); 
  ledcAttachPin(TFT_LED, TFT_PWM_Channel);
//set brightness
tft_setbrightness(200);
//ledcWrite(TFT_PWM_Channel, TFT_Brightness);

#endif   
}

void tft_setbrightness(int brightness)
{
    ledcWrite(TFT_PWM_Channel, brightness);
}
void tft_clrscrn()
{
#ifdef USE_TFT_DISPLAY 

  tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(0); 
#endif

}

void TFT_Page_Update()
{
#ifdef USE_TFT_DISPLAY
    // Take a fast snapshot of shared variables to avoid holding mutex during SPI drawing
    bool local_WXS_Online = false;
    bool local_WeeWX_Valid = false;
    String local_TempC = "";
    String local_Humidity = "";
    String local_DP = "";
    double local_DT0_Temp = 0.0;
    double local_DT1_Temp = 0.0;
    float local_Setpoint0 = 0.0;
    float local_Setpoint1 = 0.0;
    int local_pwr0 = returnPWR(ledcRead(0));
    int local_pwr1 = returnPWR(ledcRead(1));

    if (acquireDataMutex(pdMS_TO_TICKS(100))) {
        local_WXS_Online = WXS_Online;
        local_WeeWX_Valid = WeeWX_Valid;
        local_TempC = TempC;
        local_Humidity = Humidity;
        local_DP = DP;
        local_DT0_Temp = DT[0].Temperature;
        local_DT1_Temp = DT[1].Temperature;
        local_Setpoint0 = Setpoint0;
        local_Setpoint1 = Setpoint1;
        releaseDataMutex();
    }

    //Update TFT display pages here
    tft.setTextSize(1); 
//Line 1: Check if WX Station is online
    tft.setCursor(0, 0);
    if (local_WXS_Online) 
    {
        tft.setTextColor(ST7735_GREEN,ST7735_BLACK);
        tft.println(F("WX Station Online    "));
    } 
    else 
    {
        tft.setTextColor(ST7735_RED,ST7735_BLACK);
        tft.println(F("WX Station Offline!"));
    }

    tft.setTextColor(ST7735_WHITE,ST7735_BLACK);
//Line 2 to 4: Garden Temperature, Humidity, Dew Point
    if (!local_WeeWX_Valid)  //No data received yet - placeholders 
    {
        tft.setTextSize(1); 
        tft.setCursor(0, 14);
        tft.setTextWrap(true); // Allow text to run off right edge
        tft.setTextColor(ST7735_RED,ST7735_BLACK);   tft.println(F("Awaiting Data!"));
        tft.setTextWrap(false); // Allow text to run off right edge
    } 
    else 
    {
        tft.setTextColor(ST7735_GREEN,ST7735_BLACK);
        tft.print(F("Temp: "));
        tft.print(local_TempC.toFloat(), 1);
        tft.print((char)247); // Print degree symbol
        tft.print(F("C / "));
        tft.print(local_Humidity.toFloat(), 1);
        tft.println(F("%           "));
        tft.print(F("Dew Point: "));
        tft.print(local_DP.toFloat(), 2);
        tft.print((char)247); // Print degree symbol
        tft.println(F("C           "));
    }

    tft.drawFastHLine(0, 44-18, 128, ST7735_MAGENTA); //Horizontal line
//Dew Band Outputs
    tft.setCursor(1, 48-18);
    tft.setTextColor(ST7735_GREEN,ST7735_BLACK);
    tft.setTextSize(2); 
 //Dew Band Output 1 
    tft.print(F("Guide:"));
    tft.print(local_pwr0);
    tft.println(F("%           "));
    tft.setCursor(0, 68-18);
    tft.print(local_DT0_Temp, 1);
    tft.print(F("/"));
    tft.print(local_Setpoint0, 1);
    tft.print((char)247); // Print degree symbol
    tft.print(F("C   "));
 
//Dew Band Output 2
    tft.drawFastHLine(0, 88-9, 128, ST7735_MAGENTA); //Horizontal line
    tft.setCursor(0, 93-9);
    tft.print("Main:");
    tft.print(local_pwr1);
    tft.println(F("%    "));

    tft.setCursor(0, 114-9);
    tft.print(local_DT1_Temp, 1);
    tft.print(F("/"));
    tft.print(local_Setpoint1, 1);
    tft.print((char)247); // Print degree symbol
    tft.print(F("C   "));
#endif
} 