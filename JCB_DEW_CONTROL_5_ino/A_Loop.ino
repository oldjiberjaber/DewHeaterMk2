
void loop(void) 
{
/////////////////////////////////////////
// Print stuff out to the serial port on a regular basis using Serial_Update_ms time period
//
if (millis()-Serial_Update >=  Serial_Update_ms) 
 { 
  UpdateJSONData(); 
  Serial_Update =millis();    // reset timer
 }
///////////////////////////////////////// 
// Poll sensors and update values using Sensor_Update_ms time period
// Humidity, temperature and brightness
//
if (millis()-Sensor_Update >=  Sensor_Update_ms)
 {  
    Poll_Sensors();
    if (DT[0].Present){ Heater0.Compute(); } else {HeaterOutput0 = 0;};
    if (DT[1].Present){ Heater1.Compute(); } else {HeaterOutput1 = 0;};
    if (DT[2].Present){ Heater2.Compute(); } else {HeaterOutput2 = 0;};
    if (DT[3].Present){ Heater3.Compute(); } else {HeaterOutput3 = 0;};

    analogWrite(Heater0_PIN,HeaterOutput0);
    analogWrite(Heater1_PIN,HeaterOutput1);
    analogWrite(Heater2_PIN,HeaterOutput2);
    analogWrite(Heater3_PIN,HeaterOutput3);

    Sensor_Update =millis();    // reset timer
 }
///////////////////////////////////////// 
// Update LCD based on LCD_Update_ms time period
// and set brightness of LCD
//
if (millis()-LCD_Update >=  LCD_Update_ms) 
 { 
  UpdateLCDData(); 
  LCD_Update =millis();    // reset timer
  };

} // END OF LOOP     
