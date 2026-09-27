////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Churn out all data to the serial port
void UpdateSerialPortData(void)
{
   for( int i = 0; i < 4; i++) {
        Serial.print(i);
        Serial.write('\t');
        Serial.print(DT[i].Present);
        Serial.write('\t');
        Serial.print(DT[i].addr0, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr1, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr2, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr3, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr4, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr5, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr6, HEX);
        Serial.write(' ');
        Serial.print(DT[i].addr7, HEX);
        Serial.write('\t');
        Serial.print(DT[i].Temperature);
        Serial.write('\t');
        Serial.print(DT[i].SensorNum);
        Serial.println();
       }
    Serial.print("DHT");
    Serial.write('\t');
    Serial.print(DH.Present);
    Serial.print('\t');
    Serial.print(DH.Status);
    Serial.write('\t');
    Serial.write('\t');
    Serial.write('\t');
    Serial.print(DH.Temperature);
    Serial.write('\t');
    Serial.write('\t');
    Serial.print(DH.Humidity);
    Serial.write('\t');
    Serial.print(DH.DewPoint);
    Serial.println();

};
