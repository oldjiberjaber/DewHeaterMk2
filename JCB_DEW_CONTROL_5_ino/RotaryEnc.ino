/*
** Based on the Rotary Encoder Example which used the Sparkfun Rotary Encoder to vary brightness of LED
** Sample the encoder at 200Hz using the millis() function
*/

float DPOffset = 1;    // how bright the LED is, start at half brightness
float DPOffset_step = 0.5;    // how many points to fade the LED by
unsigned long currentTime;
unsigned long loopTime;
int pin_sw = 4;  // pin 13 - switch input
int pin_A =12;  // pin 12
int pin_B =11;  // pin 11
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;

void setup_enc(void)
{
  // declare pin 9 to be an output:
  pinMode(pin_A, INPUT_PULLUP);
  pinMode(pin_B, INPUT_PULLUP);
  pinMode(pin_sw, INPUT_PULLUP);
  currentTime = millis();
  loopTime = currentTime; 
} ;

void read_enc(void)  
{
  currentTime = millis();
  loopTime = currentTime; 
  clearLCD();
  goTo(0,0);
  LCD.print("Set DP Targ O/S");
  goTo(1,0);
  LCD.print("Offset =");
  goTo(1,10);
  LCD.print(DewPoint_offset,1);
  // Other processing can be done here
 ENC_Update = millis();
 while (true)
 { 
  // get the current elapsed time
  currentTime = millis();
  if(currentTime >= (loopTime + 5)){
    // 5ms since last check of encoder = 200Hz  
    encoder_A = digitalRead(pin_A);    // Read encoder pins
    encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      // A has gone from high to low 
      if(encoder_B) {
        // B is high so clockwise
        // increase the DPOffset, dont go over 20
        if(DewPoint_offset + DPOffset_step <= 20) DewPoint_offset += DPOffset_step;               
      }   
      else {
        // B is low so counter-clockwise      
        // decrease the brightness, dont go below 0
        if(DewPoint_offset - DPOffset_step >= 0) DewPoint_offset -= DPOffset_step;               
      }   

    }   
    encoder_A_prev = encoder_A;     // Store value of A for next time    
  
  goTo(1,10);
  LCD.print(DewPoint_offset,1);
 
    loopTime = currentTime;  // Updates loopTime
    }
if (millis()-ENC_Update >=  ENC_Update_ms) {break;};



};
return;

};
