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

   delay(900);     // maybe 750ms is enough, maybe not
   // we might do a ds.depower() here, but the reset will take care of it.

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
//////////////////////////////////////////////////////////////////////////////////////////
char* tempToAscii(double temp)
{
   char ascii[32];
   int frac;
   frac=(unsigned int)(temp*1000)%1000;  //get three numbers to the right of the deciaml point
  itoa((int)temp,ascii,10);
   strcat(ascii,".");
   itoa(frac,&ascii[strlen(ascii)],10); //put the frac after the deciaml

   return ascii;
}
//////////////////////////////////////////////////////////////////////////////////////////
