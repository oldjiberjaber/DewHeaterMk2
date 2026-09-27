////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// find sensors
  void Find_Sensors(void)
{  
  //init device count
  int dev_count=0;
  clearLCD(); 
  delay(500);
  goTo(0,0);
  LCD.print("Sensor Search"); 
  ds.reset_search();
  delay(500);
  int SenNo=10; 
  for( int i = 0; i < 4; i++) {
    if ( ds.search(&DT_temp.addr0))
    {
            dev_count++;
            switch (DT_temp.addr1)
                {
                 //sensor for Heater 0
                case 0xF5:
                {
                SenNo = 0;
                DT[SenNo].Present = true; 
                DT[SenNo].addr0 = DT_temp.addr0; DT[SenNo].addr1 = DT_temp.addr1; DT[SenNo].addr2 = DT_temp.addr2; DT[SenNo].addr3 = DT_temp.addr3; 
                DT[SenNo].addr4 = DT_temp.addr4; DT[SenNo].addr5 = DT_temp.addr5; DT[SenNo].addr6 = DT_temp.addr6; DT[SenNo].addr7 = DT_temp.addr7; 
                DT[SenNo].Temperature=0; DT[SenNo].SensorNum = 1;
                Serial.println();
                Serial.print(F("Found Heater 0"));
                Serial.println();
                goTo(1,0);
                LCD.print(F("Found Heater 0")); 
                delay(1000);
                break;

                }
                //sensor for Heater 1
                case 0xB0:
                {
                SenNo = 1;
                DT[SenNo].Present = true; 
                DT[SenNo].addr0 = DT_temp.addr0; DT[SenNo].addr1 = DT_temp.addr1; DT[SenNo].addr2 = DT_temp.addr2; DT[SenNo].addr3 = DT_temp.addr3; 
                DT[SenNo].addr4 = DT_temp.addr4; DT[SenNo].addr5 = DT_temp.addr5; DT[SenNo].addr6 = DT_temp.addr6; DT[SenNo].addr7 = DT_temp.addr7; 
                DT[SenNo].Temperature=0; DT[SenNo].SensorNum = 2;
                Serial.println();
                Serial.print(F("Found Heater 1"));
                Serial.println();
                goTo(1,0);
                LCD.print(F("Found Heater 1")); 
                delay(1000);
                break;
                                
                }
                //sensor for Heater 2
                case 0x3A:
                {
                SenNo=2;
                DT[SenNo].Present = true; 
                DT[SenNo].addr0 = DT_temp.addr0; DT[SenNo].addr1 = DT_temp.addr1; DT[SenNo].addr2 = DT_temp.addr2; DT[SenNo].addr3 = DT_temp.addr3; 
                DT[SenNo].addr4 = DT_temp.addr4; DT[SenNo].addr5 = DT_temp.addr5; DT[SenNo].addr6 = DT_temp.addr6; DT[SenNo].addr7 = DT_temp.addr7; 
                DT[SenNo].Temperature=0; DT[SenNo].SensorNum = 3;
                Serial.println();
                Serial.print(F("Found Heater 2"));
                Serial.println();
                goTo(1,0);
                LCD.print(F("Found Heater 2")); 
                delay(1000);
                break;
                }
                default:
                {
//                SenNo=0;
        Serial.println();
        Serial.print(F("Found Heater NONE!"));
        Serial.print(DT_temp.addr0, HEX);Serial.print(F(" "));Serial.print(DT_temp.addr1, HEX);Serial.print(F(" "));
        Serial.print(DT_temp.addr2, HEX);Serial.print(F(" "));Serial.print(DT_temp.addr3, HEX);Serial.print(F(" "));
        Serial.print(DT_temp.addr4, HEX);Serial.print(DT_temp.addr5, HEX);        Serial.println();
        delay(500);
        break;
        }
        }
        } else { 
        break;
      }
  for( int i = 0; i < 4; i++) {
  if (!DT[i].Present) {
  DT[i].Present = false; DT[i].addr0=0; DT[i].addr1=0; DT[i].addr2=0; DT[i].addr3=0; DT[i].addr4=0; DT[i].addr5=0; DT[i].addr6=0; DT[i].addr7=0; DT[i].Temperature=0; DT[i].SensorNum = 0;
  }
  };

  }
   delay(500);
 clearLCD();
  // Display devices found on LCD
  goTo(0,0);
  LCD.print(F("DT18B20 Found: ")); 
  LCD.print(dev_count); 
  goTo(1,0);
  LCD.print(F("DHT22: ")); 
  
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading DHT sensor");
    DH.Present = false;
    DH.Status=="ERR";
  }
  else {
    DH.Status=="OK";
    DH.Present = true;
   }
  
  if(DH.Status=="OK")
  {
    DH.Present = true; 
    LCD.print(DH.Status);
  } else 
  {
    DH.Present = false;
    LCD.print(DH.Status);
  };
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Poll all sensors
void Poll_Sensors(void)
{ 
double old_temp = 0;  
  for( int i = 0; i < 4; i++) {
    old_temp=DT[i].Temperature;
    if (!getOneWireTemp(&DT[i].addr0,&DT[i].Temperature,&ds))
     {
//   If we are here there is an error reading DT sensor
     DT[i].Present = false;
     DT[i].Temperature = NAN;
     } else {DT[i].Present = true;}
      if (isnan(DT[i].Temperature)) // Detect error reading whilst using sensor and re-use old value
      {
        DT[i].Temperature=old_temp;
        DT[i].errorcount++;
//        DT[i].Temperature=99; //uncomment to make errors easier to see on serial dump
      }
     }

// Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading DHT sensor");
    DH.Present = false;
    DH.Status=="ERR";
  }
  else {
    DH.Status=="OK";
    DH.Present = true;
    //Serial.print(F("Temperature: "));
    //Serial.print(event.temperature);
    //Serial.println(F("°C"));
    DH.Temperature = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading DHT sensor");
    DH.Present = false;
    DH.Status=="ERR";
  }
  else {
    DH.Status=="OK";
    DH.Present = true;
    //Serial.print(F("Humidity: "));
    //Serial.print(event.relative_humidity);
    //Serial.println(F("%"));
    DH.Humidity = event.relative_humidity;
  }


  // Calc Dew Point
  //check we have numbers
  //calculate dew point
  DH.DewPoint = DH.Temperature-((100-DH.Humidity)/5);
  DH.DewPointTarget = DH.DewPoint + DewPoint_offset ;
  if (DH.DewPointTarget > 25) DH.DewPointTarget = 25;
  LCDLDRVal = map(analogRead(LDR_Pin),0,1024,2,30); // Scale read from light sensor to 2 to 30 brightness command for display. Min level 2
  VINVal = map(analogRead(VIN_Pin),0,1024,0,15000); // Scale read from Vin to 0 to 15000mV
};
