

void setup(void) {
   // initialize inputs/outputs
   // start serial port
  Serial.begin(115200); //for serial monitor of stuff
  LCD.begin(9600);
  clearLCD();
  delay(500);
  dht.begin();
  // init DHT22 storage array
  DH.Present= false;
  DH.Humidity= 0;
  DH.Temperature = 0;
  DH.DewPoint = 0;
  DH.DewPointTarget = 0;
  LCD.print(F("Powered Up"));
  delay(500);
  setBrightness(10);
  //Serial.println(F("Index\tPresent\tAddress\t\t\tTemperature (C)\tHumidity (%)"));
  //Search for sensors
  Find_Sensors();
  //read values from sensors
  Poll_Sensors();
  // Announce to Serial Port
  UpdateSerialPortData();
  delay(2000);
////////////////////////////////
  pinMode (Heater0_PIN,OUTPUT); // setup PWM pin for Heater 1 as output
  pinMode (Heater1_PIN,OUTPUT); // setup PWM pin for Heater 2 as output
  pinMode (Heater2_PIN,OUTPUT); // setup PWM pin for Heater 3 as output
  pinMode (Heater3_PIN,OUTPUT); // setup PWM pin for Heater 4 as output
//Turn on heater controllers
//turn the PIDs on
  Heater0.SetMode(AUTOMATIC);
  Heater1.SetMode(AUTOMATIC);
  Heater2.SetMode(AUTOMATIC);
  Heater3.SetMode(AUTOMATIC);
////////////////////////////////
// Start timers for various time triggered events
//
  Serial_Update = millis();
  Sensor_Update = millis();
  LCD_Update    = millis();  
////////////////////////////////
  clearLCD(); 
////////////////////////////////
// Setup the imputs for the PIDs
////////////////////////////////
//setup rotary encoder
setup_enc();

//// Create JSON Output object
//CreateJSONData();

} // End of setup
