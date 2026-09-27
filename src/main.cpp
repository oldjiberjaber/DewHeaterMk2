#include <Arduino.h>
#include "RTOS_Config.h"

// Global Mutex for FreeRTOS
SemaphoreHandle_t xDataMutex = NULL;

// Global flag to indicate OTA update in progress
volatile bool isOTAInProgress = false;

#define SW_VERSION "001"
#define COMPILE_DATE __DATE__
#define COMPILE_TIME __TIME__

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>  //new from V030
#include <ESPAsyncWebServer.h> //new from V030
#include <WebSerial.h>
#include "LittleFS.h"
#include <string.h>
#include <PubSubClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Adafruit_GFX.h>    // Core graphics library

#include <SPI.h>
#include <Adafruit_ST7735.h> // Hardware-specific library

#include <Wire.h> //IIC library
#include <Adafruit_SH110X.h>   // Adafruit SH110X display driver (supports SH1107) 128x128 OLED IIC display library

#include <QuickPID.h>
#include <sTune.h>
#include <Preferences.h> // Native ESP32 library for saving data

#include <ArduinoJson.h>
#define ARDUINOJSON_USE_LONG_LONG 1

const String mqtt_ID = "dewhtr"; 
const char* WIFIID = "dewhtr";

//Define which display we are using please!
#define USE_TFT_DISPLAY

/*********************************************
   Pin Definitions
01
02
03
04 One wire data pin
05
06
07
08
09
10
11
12 TFT_CS
13
14 TFT_DC
15
16 MOSFET1 
17 MOSFET2 
18 SCK
19 TFT_LED
20
21
22
23 MOSI
24
25
26 MOSFET3
27 MOSFET4
28
29
30
 *********************************************/

// TFT display pins and instance
#define TFT_CS     12 //16
#define TFT_RST    -1  // you can also connect this to the Arduino reset
// in which case, set this #define pin to -1!
#define TFT_DC     14 //17
#define TFT_LED    19 
//const int mosfetPins[] = {13, 12, 14, 27}; 
const int mosfetPins[] = {16, 17, 26, 27}; 
const int TFT_PWM_Channel = 10;
int TFT_Brightness = 20; //default TFT display brightness (8bit)
void tft_setbrightness(int brightness);
void tft_clrscrn();
/*********************************************
   Instances
 *********************************************/
WiFiClient client;
// MQTT Specifics
PubSubClient mqttclient = client;
#ifdef USE_TFT_DISPLAY
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#endif 

// Create AsyncWebServer object on port 80 & 81
AsyncWebServer server(80);
AsyncWebServer server2(81);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
StaticJsonDocument<1024> readings;

//Onewire Setup
OneWire  ds(4);  // on GPIO 04 (a 4.7K resistor is necessary)

#include "AAA_Credentials.h"
#include "SolarDimmer.h"
SolarDimmer solarDimmer(LOC_LATITUDE, LOC_LONGITUDE);
#include "AAAAA_Debugging.h"
#include "AAAA_JSON.h"
#include "Arducpp_OTA.h"

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

DT_Sensor DT[4] = { 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
{ false, 0, 0, 0, 0, 0, 0,0,0,0,0,0}, 
};
DT_Sensor DT_temp = { false, 0, 0, 0, 0, 0, 0,0,0,0,0};

//const int mosfetPins[] = {13, 12, 14, 27}; 
const int numChannels = 4;
// Generally, you should use "unsigned long" for variables that hold time
unsigned long lastReconnectAttempt = 0;
unsigned long WXStationOnlineTimer = 0;
unsigned int MqttCount = 0;
bool WXS_Online = false;

// Function prototypes
void mqttclient_pub(String MQTTTOPIC, String MQTTMSG, boolean mqttdebug = true, boolean mqttpersistence = false);
void WIFI_connection(void);
void Arduino_OTA();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
boolean reconnect();
int WifiGetRssiAsQuality(int rssi);

void MQTT_Debug(char* topic, String msgString, unsigned int length);
void TFT_Page_Update();
int returnPWR(int PWMVal);
String Humidity, TempC, Pressure, DP, UV, Rain1h, Rain24h, Pressuretrend;
float DP_Offset0 = 4.0;
float DP_Offset1 = 4.0;
int pwmDuty[numChannels] = {0, 0, 0, 0};

void Find_Sensors(void);
boolean getOneWireTemp(byte *addr, double *retnTemp, OneWire * OW);
void Poll_Sensors(void);




#include "..\lib\WiFiConnection.cpp"
#include "..\lib\MQTT.cpp"
#include "..\lib\PID.cpp"
#include "..\lib\tft.cpp"
#include "..\lib\onewire.cpp"
#include "..\lib\web.cpp"



// Task Prototypes
void TaskSensors(void *pvParameters);
void TaskPID(void *pvParameters);
void TaskDisplay(void *pvParameters);
void TaskNetwork(void *pvParameters);

void setup() {
  Serial.begin(115200); //Physical Serial Port

  // 1. Immediately drive all MOSFET PWM heater channels to 0% OFF on boot
  for (int i = 0; i < numChannels; i++) {
    ledcSetup(i, 5000, 8); 
    ledcAttachPin(mosfetPins[i], i);
    ledcWrite(i, 0);
    pwmDuty[i] = 0;
  }
  Output0 = 0;
  Output1 = 0;

  // Initialize FreeRTOS Mutex
  xDataMutex = xSemaphoreCreateMutex();

  Serial.println(F(""));
  Serial.println(F(""));
  Serial.print(F("Arduino ESP32 Code Version: "));
  Serial.println(SW_VERSION);
  Serial.print(F("Compiled on: "));
  Serial.print(COMPILE_DATE);
  Serial.print(F(" "));
  Serial.println(COMPILE_TIME);Serial.println();

  init_tft();
//TFT Init

  tft.println(F("Dew Controller 2026"));
  tft.println(F("Arduino ESP32 Code"));
  tft.print(F("Version: "));
  tft.println(SW_VERSION);
  tft.println(F("Compiled on:"));
  tft.print(COMPILE_DATE);
  tft.print(F(" "));
  tft.println(COMPILE_TIME);Serial.println();
  //Start WiFi
  WIFI_connection();

  // Initialize Solar Dimmer from persisted NVS settings
  solarDimmer.begin(LOC_LATITUDE, LOC_LONGITUDE);

  // Initialize NTP time sync and solar dimming calculations
  solarDimmer.initNTP(TIMEZONE_TZ, NTP_SERVER_1, NTP_SERVER_2);

  WebSerial.begin(&server2); //Web based Serial Port on Port 81
  //WebSerial.msgCallback(recvMsg);
  server2.begin();

  WebSerial.println(F(""));
  WebSerial.println(F(""));
  WebSerial.print(F("Arduino ESP32 Code Version: "));
  WebSerial.println(SW_VERSION);
  WebSerial.print(F("Compiled on: "));
  WebSerial.print(COMPILE_DATE);
  WebSerial.print(F(" "));
  WebSerial.println(COMPILE_TIME);WebSerial.println();

  //Start OTA Client
  Arduino_OTA();

  //Start MQTT stuff (only in Station Mode)
  if (!isAPMode) {
    init_mqtt();
  }

  delay(500);
  tft_clrscrn();
  //Search Sensors and announce them
  tft.println(F("Searching Sensors"));
  Serial.println(F("Searching Sensors"));
  //Search for sensors
  Find_Sensors();
  //read values from sensors
  Poll_Sensors();
  loadOffsetsFromNVS();
  SetHeaterValues();  //Set Temperature, target for PID 
  delay(200);
  
  tft.println(F("MOSFETs: All 0% OFF"));
  Serial.println(F("MOSFETs: All 0% OFF"));
  delay(200);

  //Calculate initial settings
  tft.println(F("Calculating Settings"));
  Serial.println(F("Calculating Settings"));
  // PID Setup 
  initPID();

  String PID_Setting= "H0:" + String(H0_Kp) + "," + String(H0_Ki) + "," + String(H0_Kd);
  tft.println(PID_Setting);
  Serial.println(PID_Setting);
  PID_Setting= "H1:" + String(H1_Kp) + "," + String(H1_Ki) + "," + String(H1_Kd);
  tft.println(PID_Setting);
  Serial.println(PID_Setting);

  delay(700);
  //start webserver and such
  tft.println(F("Sarting Webserver"));
  Serial.println(F("Sarting Webserver"));
  initWebServer();
  // Start server
  server.begin();

  tft.println(F("Webserver Started"));
  Serial.println(F("Webserver Started"));
  delay(200);

  tft.println(F("!!! INITALISED !!!"));
  delay(500);
  Serial.println("Initialised");
  tft_clrscrn();

  //set TFT brightness to default level
  tft_setbrightness(TFT_Brightness);

  // ---------------- Launch FreeRTOS Tasks ----------------
  // Core 1 Tasks: Real-time sensor reading and PID control
  xTaskCreatePinnedToCore(TaskSensors, "SensorsTask", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskPID, "PIDTask", 4096, NULL, 3, NULL, 1);

  // Core 0 Tasks: Display rendering and Network / Telemetry handling
  xTaskCreatePinnedToCore(TaskDisplay, "DisplayTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskNetwork, "NetworkTask", 4096, NULL, 1, NULL, 0);

  Serial.println(F("FreeRTOS Tasks Successfully Launched on Core 0 and Core 1"));
}

// ---------------- FreeRTOS Task Implementations ----------------

// Sensor polling task on Core 1 (Priority 2)
void TaskSensors(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    if (!isOTAInProgress) {
      Poll_Sensors();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Deterministic PID Heater Control Task on Core 1 (Priority 3 - High)
void TaskPID(void *pvParameters) {
  (void) pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(200); // Strict 200ms cycle

  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    if (isOTAInProgress) {
      ledcWrite(0, 0);
      ledcWrite(1, 0);
      continue;
    }

    if (acquireDataMutex(pdMS_TO_TICKS(50))) {
      SetHeaterValues();  // Set Temperature, target for PID
      PIDMagic();

      if (DT[0].Present) {
        ledcWrite(0, Output0);  // Output from PID to CH0 (Guide Scope)
      } else {
        ledcWrite(0, 0);
      }

      if (DT[1].Present) {
        ledcWrite(1, Output1);  // Output from PID to CH1 (Main Scope)
      } else {
        ledcWrite(1, 0);
      }
      releaseDataMutex();
    }
  }
}

// Display Task on Core 0 (Priority 1)
void TaskDisplay(void *pvParameters) {
  (void) pvParameters;
  unsigned long lastDimmerCheck = 0;

  for (;;) {
    if (!isOTAInProgress) {
      // Periodically check Solar position and adjust screen dimming (every 5 seconds)
      if (millis() - lastDimmerCheck > 5000) {
        lastDimmerCheck = millis();
        if (solarDimmer.autoDimEnabled) {
          SolarInfo sInfo = solarDimmer.calculateSolar();
          if (sInfo.timeValid) {
            if (TFT_Brightness != sInfo.targetBrightness) {
              TFT_Brightness = sInfo.targetBrightness;
              tft_setbrightness(TFT_Brightness);
            }
          }
        }
      }

      TFT_Page_Update();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Network, MQTT & Web Telemetry Task on Core 0 (Priority 1)
void TaskNetwork(void *pvParameters) {
  (void) pvParameters;
  unsigned long lastTelemetryUpdate = 0;

  for (;;) {
    // 1. Service OTA with top priority
    ArduinoOTA.handle();

    if (isOTAInProgress) {
      // While flashing firmware/filesystem, prioritize OTA packet reception
      vTaskDelay(1);
      continue;
    }

    if (shouldReboot) {
      if (learningModelModified) {
        saveLearningToNVS();
      }
      Serial.println("Rebooting...");
      vTaskDelay(pdMS_TO_TICKS(100));
      ESP.restart();
    }

    if (isAPMode) {
      dnsServer.processNextRequest();
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Housekeeping
    mqttclient.loop();

    // WiFi connection watchdog & reconnect
    static unsigned long wifiDisconnectedSince = 0;
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDisconnectedSince == 0) {
        wifiDisconnectedSince = millis();
        Serial.println(F("WiFi connection lost. Attempting reconnect..."));
        WiFi.reconnect();
      } else if (millis() - wifiDisconnectedSince > 45000) {
        // If disconnected for more than 45 seconds continuously, restart
        Serial.println(F("WiFi reconnection failed for >45s. Rebooting..."));
        ESP.restart();
      }
    } else {
      wifiDisconnectedSince = 0;
    }

    // Non-blocking MQTT reconnect
    if (millis() - lastReconnectAttempt > 20000) {
      if (!mqttclient.connected()) {
        if (reconnect()) {
          lastReconnectAttempt = 0;
        }
      }
      lastReconnectAttempt = millis();
    }

    // Telemetry output & MQTT publishing every 1 second
    if (millis() - lastTelemetryUpdate > 1000) {
      lastTelemetryUpdate = millis();

      String SerialSensorOutput = "";
      String topic0 = "telescope/dew0";
      String topic1 = "telescope/dew1";
      String payload0 = "";
      String payload1 = "";
      bool dt0_present = false;
      bool dt1_present = false;

      if (acquireDataMutex(pdMS_TO_TICKS(100))) {
        dt0_present = DT[0].Present;
        dt1_present = DT[1].Present;

        if (dt0_present) {
          SerialSensorOutput = String(millis() / 1000) + ": Temp0: " + String(Input0) + "C | Target0: " + String(Setpoint0) + "C | PWR0: " + String(returnPWR(ledcRead(0))) + "%";
          jsonH0["Temp"] = Input0;
          jsonH0["Target"] = Setpoint0;
          jsonH0["PWR"] = returnPWR(ledcRead(0));
          payload0 = jsonH0.as<String>();
        }

        if (dt1_present) {
          if (dt0_present) {
            SerialSensorOutput += " | Temp1: " + String(Input1) + "C | Target1: " + String(Setpoint1) + "C | PWR1: " + String(returnPWR(ledcRead(1))) + "%";
          } else {
            SerialSensorOutput = String(millis() / 1000) + ": Temp1: " + String(Input1) + "C | Target1: " + String(Setpoint1) + "C | PWR1: " + String(returnPWR(ledcRead(1))) + "%";
          }
          jsonH1["Temp"] = Input1;
          jsonH1["Target"] = Setpoint1;
          jsonH1["PWR"] = returnPWR(ledcRead(1));
          payload1 = jsonH1.as<String>();
        }
        releaseDataMutex();
      }

      if (dt0_present && payload0.length() > 0) {
        mqttclient_pub(topic0, payload0);
      }
      if (dt1_present && payload1.length() > 0) {
        mqttclient_pub(topic1, payload1);
      }

      if (SerialSensorOutput.length() > 0) {
        Serial.println(SerialSensorOutput);
        WebSerial.println(SerialSensorOutput);
      }

      events.send("ping", NULL, millis());
      events.send(getSensorReadings().c_str(), "new_readings", millis());
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // Yield CPU to other tasks on Core 0
  }
}

void loop() {
  // Main loop simply yields to allow background tasks to run smoothly
  vTaskDelay(pdMS_TO_TICKS(1000));
}






/////////////////////////////////////////////////////////////////////////////////////
