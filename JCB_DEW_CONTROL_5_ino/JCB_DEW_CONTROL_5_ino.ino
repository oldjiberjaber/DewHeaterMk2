#include <ArduinoJson.h>

#include <math.h>
#include <PID_v1.h>
#include <OneWire.h> // Dallas one wire lib
//#include "DHT.h" //DHT Lib for DHT22 temp & Humidity sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <SoftwareSerial.h>
#include "LCD.h"

///////////////////////////////////////////////////////////////////////
// Target Board
// Arduino Pro Mini
// 328P 16MHz 5V 
//
///////////////////////////////////////////////////////////////////////
// Pin Assignments
//
//
// A0 - LDR measurement input (TEPT5700)
// A1 - Vin measurement input
// A2 - N/C
// A3 - N/C
// D0 - RXI FTDI
// D1 - TXO FTDI
// D2 - N/C
// D3 - Heater 0 PWM
// D4 - pin_sw input from encoder
// D5 - Heater 1 PWM 
// D6 - Heater 2 PWM
// D7 - LCD Serial Port TX
// D8 - DHT22 Device interface
// D9 - Heater 3 PWM
// D10 - Dallas one wire interface
// D11 - pin_B input from encoder
// D12 - pin_A input from encoder
// D13 - LCD Serial Port RX

// DS18B20 sensor 2: {0x28,0xF5,0xF7,0xE4,0x04,0x00,0x00,0xA9} //heater 0 sensor
// DS18B20 sensor 1: {0x28,0xB0,0xC4,0xE4,0x04,0x00,0x00,0xFD} //heater 1 sensor
// DS18B20 sensor 3: {0x28,0x3A,0x66,0xE5,0x04,0x00,0x0E,0x07) //spare
// DS18B20 Temperature chip i/o
OneWire  ds(10);  // Dallas 1-wire on pin 10
#define DHTPIN 8     // Digital pin connected to the DHT sensor 
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

//DHT dht; //create our DHT device

///////////////////////////////////////////////////////////////////////
// LDR for brightness adjustment of LCD display
//
int LDR_Pin = A0; //Read LDR signal from analog pin 0
int LCDLDRVal = 0; //value read from LDR pin, polled everytime the other sensors are polled.
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// Vin Monitor uses a 20k/10k resistor divider so FSD @5v = 15V
//
int VIN_Pin = A1; //Read Vin signal from analog pin 1
int VINVal = 0; //value read from Vin pin, polled everytime the other sensors are polled.
///////////////////////////////////////////////////////////////////////
// Sensor arrays
//
typedef struct
  {
	boolean  Present;
        byte addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7;
        double Temperature;
        int errorcount;
        int SensorNum; // 0 = not present, 1 = heater 1, 2 = heater 2 (add more if required)
   }DT_Sensor;

typedef struct
  {
	String Status;
        boolean  Present;
        double Humidity, Temperature, DewPoint, DewPointTarget;
  } DH_Sensor;

DH_Sensor DH; 

DT_Sensor DT[4] = { 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
};
DT_Sensor DT_temp = { false, 0, 0, 0, 0, 0, 0,0,0,0,0};
///////////////////////////////////////////////////////////////////////
// Timers for timer triggered events / routines 
//
#define Serial_Update_ms 500 // seconds between serial prints
#define LCD_Update_ms 200 // seconds between LCD updates
#define Sensor_Update_ms 250 // seconds between sensor updates
#define ENC_Update_ms 10000 // seconds between sensor updates
unsigned long Serial_Update,Sensor_Update,LCD_Update,ENC_Update;
int page = 0; // Page flag for LCD
///////////////////////////////////////////////////////////////////////
//
// PID control of heaters
//
///////////////////////////////////////////////////////////////////////
//Specify the links and initial tuning parameters
//Input: The variable we're trying to control (double) 
//Output: The variable that will be adjusted by the pid (double) 
//Setpoint: The value we want to Input to maintain (double) 
//Kp, Ki, Kd: Tuning Parameters. these affect how the pid will chage the output. (double>=0) 
//Kp = increased value gives more proportional action and faster control
//Ki = decreased value gives more integral action and faster control - the speed it gets to teh set point after reaching the value, too much may cause osciliation
//Kd = increased value gives more derivative action and faster control - this is the control for the size of change applied (a step value)
//Direction: Either DIRECT or REVERSE. determines which direction the output will move when faced with a given error. DIRECT is most common. 

//Define Variables we'll be connecting to
double HeaterOutput0, HeaterOutput1,HeaterOutput2, HeaterOutput3;



float  DewPoint_offset = 15; //this is the offset we will target with the heater above the dew point

//A PID for each heater
PID Heater0(&DT[0].Temperature, &HeaterOutput0, &DH.DewPointTarget,30,50,5, DIRECT);
PID Heater1(&DT[1].Temperature, &HeaterOutput1, &DH.DewPointTarget,30,50,5, DIRECT);
PID Heater2(&DT[2].Temperature, &HeaterOutput2, &DH.DewPointTarget,30,50,5, DIRECT);
PID Heater3(&DT[3].Temperature, &HeaterOutput3, &DH.DewPointTarget,30,50,5, DIRECT);
#define Heater0_PIN 9 // PWM Output pin for Heater 0
#define Heater1_PIN 6 // PWM Output pin for Heater 1
#define Heater2_PIN 3 // PWM Output pin for Heater 0
#define Heater3_PIN 5 // PWM Output pin for Heater 1



////////////////////////////////////////////////////////////////
// Prototypes
void UpdateSerialPortData(void);
void Poll_Sensors(void);
boolean getOneWireTemp(byte *addr, double *retnTemp, OneWire * OW);


////////////////////////////////////////////////////////////////
// JSON
const size_t capacity = 2*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4);
DynamicJsonDocument doc(capacity);
JsonObject Sensor = doc.createNestedObject("Sensor");
JsonObject DP = doc.createNestedObject("DP");
JsonObject DewBand0 = doc.createNestedObject("DewBand0");
JsonObject DewBand1 = doc.createNestedObject("DewBand1");
