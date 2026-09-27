void UpdateJSONData() 
{
Sensor["T"] = round2((double) DH.Temperature);
Sensor["H"] = round2((double) DH.Humidity);
Sensor["V"] = round2((float) VINVal/1000);

DP["DP"] = round2((double) DH.DewPoint) ;
DP["Offset"] = round2((float) DewPoint_offset);
DP["Target"] = round2((double) DH.DewPointTarget);

DewBand0["T"] = round2((double) DT[0].Temperature);
DewBand0["P"] = round2((double)100*(HeaterOutput0/255));

DewBand1["T"] = round2((double) DT[1].Temperature);
DewBand1["P"] = round2((double) 100*(HeaterOutput1/255));
//Serial.print(F("<"));
serializeJson(doc, Serial);
//Serial.println(F(">"));
}

// rounds a number to 2 decimal places
// example: round(3.14159) -> 3.14
double round2(double value) {
   return (int)(value * 100 + 0.5) / 100.0;
}

/*
 * 
 * 
 
<{"Sensor":{"T":21.8,"H":61.4,"V":12.13},"DP":{"DP":14.08,"Offset":20,"Target":34.08},"DewBand0":{"T":33.13,"P":100},"DewBand1":{"T":28.06,"P":100}}>
<{"Sensor":{"T":21.7,"H":61.4,"V":12.13},"DP":{"DP":13.98,"Offset":20,"Target":33.98},"DewBand0":{"T":33.38,"P":100},"DewBand1":{"T":28.5,"P":100}}>
<{"Sensor":{"T":21.7,"H":61.4,"V":12.13},"DP":{"DP":13.98,"Offset":20,"Target":33.98},"DewBand0":{"T":33.63,"P":99.27},"DewBand1":{"T":29,"P":100}}>
//////////////////////////////////////
//// Parsing Programme
const size_t capacity = 2*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 110;
DynamicJsonDocument doc(capacity);

const char* json = "{\"Sensor\":{\"T\":\"50.1\",\"H\":\"50.1\",\"V\":\"13.33\"},\"DP\":{\"DP\":\"50.1\",\"Offset\":\"50\",\"Target\":\"50.1\"},\"DewBand0\":{\"T\":\"50.1\",\"P\":\"255\"},\"DewBand1\":{\"T\":\"50.1\",\"P\":\"255\"}}";

deserializeJson(doc, json);

JsonObject Sensor = doc["Sensor"];
const char* Sensor_T = Sensor["T"]; // "50.1"
const char* Sensor_H = Sensor["H"]; // "50.1"
const char* Sensor_V = Sensor["V"]; // "13.33"

JsonObject DP = doc["DP"];
const char* DP_DP = DP["DP"]; // "50.1"
const char* DP_Offset = DP["Offset"]; // "50"
const char* DP_Target = DP["Target"]; // "50.1"

const char* DewBand0_T = doc["DewBand0"]["T"]; // "50.1"
const char* DewBand0_P = doc["DewBand0"]["P"]; // "255"

const char* DewBand1_T = doc["DewBand1"]["T"]; // "50.1"
const char* DewBand1_P = doc["DewBand1"]["P"]; // "255"

//////////////////////////////////////
//// Serializing  Programme
const size_t capacity = 2*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4);
DynamicJsonDocument doc(capacity);

JsonObject Sensor = doc.createNestedObject("Sensor");
Sensor["T"] = "50.1";
Sensor["H"] = "50.1";
Sensor["V"] = "13.33";

JsonObject DP = doc.createNestedObject("DP");
DP["DP"] = "50.1";
DP["Offset"] = "50";
DP["Target"] = "50.1";

JsonObject DewBand0 = doc.createNestedObject("DewBand0");
DewBand0["T"] = "50.1";
DewBand0["P"] = "255";

JsonObject DewBand1 = doc.createNestedObject("DewBand1");
DewBand1["T"] = "50.1";
DewBand1["P"] = "255";

serializeJson(doc, Serial);


*/
