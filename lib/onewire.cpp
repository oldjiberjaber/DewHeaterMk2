// DS18B20 OneWire Sensor Management//

void Find_Sensors(void)
{  
  //init device count
  int dev_count=0;
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
                case 0xCB:
                {
                SenNo = 0;
                DT[SenNo].Present = true; 
                DT[SenNo].addr0 = DT_temp.addr0; DT[SenNo].addr1 = DT_temp.addr1; DT[SenNo].addr2 = DT_temp.addr2; DT[SenNo].addr3 = DT_temp.addr3; 
                DT[SenNo].addr4 = DT_temp.addr4; DT[SenNo].addr5 = DT_temp.addr5; DT[SenNo].addr6 = DT_temp.addr6; DT[SenNo].addr7 = DT_temp.addr7; 
                DT[SenNo].Temperature=0; DT[SenNo].SensorNum = 1;
                Serial.println();
                Serial.println(F("Found Heater 0"));
                tft.println(F("Found Heater 0"));
                //Serial.println();
                break;

                }
                //sensor for Heater 1
                case 0x47:
                {
                SenNo = 1;
                DT[SenNo].Present = true; 
                DT[SenNo].addr0 = DT_temp.addr0; DT[SenNo].addr1 = DT_temp.addr1; DT[SenNo].addr2 = DT_temp.addr2; DT[SenNo].addr3 = DT_temp.addr3; 
                DT[SenNo].addr4 = DT_temp.addr4; DT[SenNo].addr5 = DT_temp.addr5; DT[SenNo].addr6 = DT_temp.addr6; DT[SenNo].addr7 = DT_temp.addr7; 
                DT[SenNo].Temperature=0; DT[SenNo].SensorNum = 2;
                Serial.println();
                Serial.println(F("Found Heater 1"));
                tft.println(F("Found Heater 1"));
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
                Serial.println(F("Found Heater 2"));
                tft.println(F("Found Heater 2"));
                break;
                }
                default:
                {
//                SenNo=0;
        Serial.println();
        Serial.print(F("Found Heater NONE!"));
        tft.println(F("Found Heater NONE!"));


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
  // Display devices found on LCD
  
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Poll all sensors
void Poll_Sensors(void)
{ 
  for( int i = 0; i < 4; i++) {
    // Only poll sensors that are known or during discovery
    if (DT[i].Present || DT[i].SensorNum > 0 || i < 2) {
      double new_temp = 0;
      boolean ok = getOneWireTemp(&DT[i].addr0, &new_temp, &ds);
      
      if (acquireDataMutex(pdMS_TO_TICKS(100))) {
        double old_temp = DT[i].Temperature;
        if (!ok) {
          DT[i].Present = false;
          DT[i].Temperature = NAN;
        } else {
          DT[i].Present = true;
          DT[i].Temperature = new_temp;
        }
        if (isnan(DT[i].Temperature)) { // Detect error reading whilst using sensor and re-use old value
          DT[i].Temperature = old_temp;
          DT[i].errorcount++;
        }
        releaseDataMutex();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
//Matt G.
//onion@doorcreekorchard.com
//July 2008
 boolean getOneWireTemp(byte *addr, double *retnTemp, OneWire * OW)
 {
   byte present = 0;
   byte data[12];
   int raw;
   double temp;

   OW->reset();
   OW->select(addr);
   OW->write(0x44,1);         // start conversion, with parasite power on at the end

   vTaskDelay(pdMS_TO_TICKS(800));     // Non-blocking RTOS delay! Yields CPU to other tasks

   present = OW->reset();
   if (present)  //only continue if the sensor is actually there and responding
   {
     OW->select(addr);    
     OW->write(0xBE);         // Read Scratchpad which now contains the temperature data
     for ( int i = 0; i < 9; i++) {           // we need 9 bytes
       data[i] = OW->read();
     }
      if ( OneWire::crc8( data, 8) != data[8]) {
         //Serial.println("CRC is not valid!\n");
         return false;
         }
     raw=(data[1]<<8)+data[0];  //put the two bytes of the temperature (from the response) into a raw int
     temp = (double)raw * 0.0625; //convert to celcius
//     *retnTemp=temp*1.8+32;//convert to fahrenheit and set the final return value
     *retnTemp=temp;//set the final return value
     return true;
   }
     return false;
}

