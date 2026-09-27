////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Churn out all data to the LCD
//
void UpdateLCDData(void)
{
  clearLCD();
  setBrightness(LCDLDRVal); // set LCD brightness
//  setBrightness(2); // set LCD brightness
/////////////////////////////////////////////////////////////
// T1: T2:
// H3: DP:
/*
  goTo(0,0);
  LCD.print("T1:");
  goTo(0,3);
  LCD.print(DT[0].Temperature,1);
  goTo(0,9);
  LCD.print("T2:");
  LCD.print(DT[1].Temperature,1);
  goTo(1,0);
  LCD.print("H3:");
  LCD.print(DH.Humidity, 1);
  LCD.print("%");
  goTo(1,9);
  LCD.print("DP:");
  LCD.print(DH.DewPoint, 1);
*/
/////////////////////////////////////////////////////////////
// T1:P1% H3:
// T2:P2% DP:
/*
  goTo(0,0);
  LCD.print("1:");
  LCD.print(DT[0].Temperature,1);
  LCD.print("/");
  LCD.print(100*(HeaterOutput0/255), 0);
  LCD.print("%");
  goTo(0,12);
  LCD.print("H:");
  LCD.print(DH.Humidity, 0);
  goTo(1,0);
  LCD.print("2:");
  LCD.print(DT[1].Temperature,1);
  LCD.print("/");
  LCD.print(100*(HeaterOutput1/255), 0);
  LCD.print("%");
  goTo(1,12);
  LCD.print("D:");
  LCD.print(DH.DewPoint, 0);
*/
/////////////////////////////////////////////////////////////
// T1:P1% Page   - flips between 3 pages on the LCD
// T2:P2% Value
// Page 0 = Humidity, 1 = Dew Temp, 2 = Dew point, 3 = dew point target, 4=Vin
//
   switch (page)
      {
       //Humidity
      case 0:
      {
       //Heater 0 & Heater 1
      goTo(0,0); LCD.print("1:"); LCD.print(DT[0].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput0/255), 0); LCD.print("%");
      goTo(1,0); LCD.print("2:"); LCD.print(DT[1].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput1/255), 0); LCD.print("%");
      goTo(0,12); LCD.print("Hum%"); 
      goTo(1,12); LCD.print(DH.Humidity, 1);
      page++; 
      break;
      }
       //Dew Sensor Temp
      case 1:
      {
        //Heater 2 & Heater 3
      goTo(0,0); LCD.print("1:"); LCD.print(DT[0].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput0/255), 0); LCD.print("%");
      goTo(1,0); LCD.print("2:"); LCD.print(DT[1].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput1/255), 0); LCD.print("%");
 //     goTo(0,0); LCD.print("3:"); LCD.print(DT[2].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput2/255), 0); LCD.print("%");
 //     goTo(1,0); LCD.print("4:"); LCD.print(DT[3].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput3/255), 0); LCD.print("%");
      goTo(0,12); LCD.print("T'C"); 
      goTo(1,12); LCD.print(DH.Temperature, 1);
      page++; 
      break;
      }
       //DP
      case 2:
      {
       //Heater 0 & Heater 1
      goTo(0,0); LCD.print("1:"); LCD.print(DT[0].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput0/255), 0); LCD.print("%");
      goTo(1,0); LCD.print("2:"); LCD.print(DT[1].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput1/255), 0); LCD.print("%");
      goTo(0,12); LCD.print("DewP"); 
      goTo(1,12); LCD.print(DH.DewPoint, 1);
      page++; 
      break;
      }
       //DP Target
      case 3:
      {
        //Heater 2 & Heater 3
      goTo(0,0); LCD.print("1:"); LCD.print(DT[0].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput0/255), 0); LCD.print("%");
      goTo(1,0); LCD.print("2:"); LCD.print(DT[1].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput1/255), 0); LCD.print("%");
 //      goTo(0,0); LCD.print("3:"); LCD.print(DT[2].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput2/255), 0); LCD.print("%");
//      goTo(1,0); LCD.print("4:"); LCD.print(DT[3].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput3/255), 0); LCD.print("%");
      goTo(0,12); LCD.print("Targ"); 
      goTo(1,12); LCD.print(DH.DewPointTarget, 1); 
      page++; 
      break;
      }
      default:
      {
       //Heater 0 & Heater 1
      goTo(0,0); LCD.print("1:"); LCD.print(DT[0].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput0/255), 0); LCD.print("%");
      goTo(1,0); LCD.print("2:"); LCD.print(DT[1].Temperature,1); LCD.print("/"); LCD.print(100*(HeaterOutput1/255), 0); LCD.print("%");
      goTo(0,12); LCD.print("Vin"); 
      goTo(1,12); LCD.print((float)VINVal/1000, 1); 
      page=0; 
      break;
      }
      };
};
