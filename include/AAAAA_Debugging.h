bool debug = 0;                        //debug = 1 -> enable debug
bool Sensordebug = 0;                        //debug = 1 -> enable debug
bool SGP30debug = 0;                        //debug = 1 -> enable debug
bool MQTTdebug = 0;                        //debug = 1 -> enable debug
bool LDRdebug = 0;                        //debug = 1 -> enable debug
bool LUXdebug = 0;                        //debug = 1 -> enable debug
bool WWWdebug = 0;                        //debug = 1 -> enable debug
bool SendNexTextdebug = 0;      //print out stuff sent to teh Nextion Display 1 = enabled
int barocount = 0;
bool ClearEEPROMnow = 0;               //Clear EEPROM = 0 -> set true (1) to clear the SAVED SETTINGS, only run this once!
bool Calibration = 0;

#define LED_GPIO   2
#define PWM1_Ch    0
#define PWM1_Res   8
#define PWM1_Freq  1000

int PWM1_DutyCycle = 0;
 
