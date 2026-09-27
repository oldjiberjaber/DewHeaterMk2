
//#include "WProgram.h" 

// Commands
#define LCD_BACKLIGHT		0x80
#define LCD_CLEARDISPLAY	0x01

// LCD stuff:
#define LCDrxPIN 13 // Serial RX on pin 13
#define LCDtxPIN 7 // Serial TX on pin 7
#define LCD_Baud 9600 // Serial baudrate for MSB 

SoftwareSerial LCD =  SoftwareSerial(LCDrxPIN, LCDtxPIN);

// wbp: goto with row & column
void goTo(int row, int col) {
  LCD.write(0xFE);   //command flag
  LCD.write((col + row*64 + 128));    //position 
  delay(5);
}
void clearLCD(void){
  LCD.write(0xFE);   //command flag
  LCD.write(LCD_CLEARDISPLAY);   //clear command.
  delay(5);
}

// Set brightness value range 1-30 1=OFF 30=FULL
void setBrightness(int val){
 if(val >= 1 && val <= 30){
  LCD.write(0x7c);   //command flag
  LCD.write(LCD_BACKLIGHT | (val - 1));   //brightness command.
  delay(5);	
}
}
