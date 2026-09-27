
extern unsigned int MqttCount;
#include <Arduino.h>  // Add this to define String, Serial, and other Arduino types

bool isLWTOnline = true;

bool setLWTState(bool online) {
  String lwtTopic = "tele/" + mqtt_ID + "/LWT";
  char MQTTtopic[lwtTopic.length() + 1];
  lwtTopic.toCharArray(MQTTtopic, lwtTopic.length() + 1);

  if (!mqttclient.connected()) {
    reconnect();
  }

  if (mqttclient.connected()) {
    const char* payload = online ? "Online" : "Offline";
    bool ok = mqttclient.publish(MQTTtopic, payload, true);
    if (ok) {
      isLWTOnline = online;
      Serial.printf(">>> LWT Manually Set: Topic='%s', State='%s' (retained)\n", MQTTtopic, payload);
    }
    return ok;
  }
  return false;
}

void init_mqtt() {
  mqttclient.setServer(mqtt_server, 1883);
  mqttclient.setBufferSize(1024);  //Increase MQTT_MAX_PACKET_SIZE using API to allow WeeWX messages to be received.
  mqttclient.setCallback(mqtt_callback);
  tft.println(F("MQTT Attempt"));
  Serial.print(F("MQTT Attempt..."));
  if (reconnect()) {lastReconnectAttempt = 0;tft.println(F("MQTT Connected!!")); } else {tft.println(F("MQTT Failed"));}
  lastReconnectAttempt = 0;
  tft.println(F("Initalised"));
  Serial.println(F("Initalised")); 
}


boolean reconnect() {
  Serial.print(F("Attempting MQTT connection..."));
  String lwtTopic = "tele/"+mqtt_ID+"/LWT";
  char MQTTtopic[lwtTopic.length()+1];
  lwtTopic.toCharArray(MQTTtopic,lwtTopic.length()+1); 
if (mqttclient.connect(WIFIID, "22", "22", MQTTtopic, 1, true, "Offline")) 
  {
  // Once connected, publish an announcement...
      mqttclient.publish(MQTTtopic,"Online",true);
      isLWTOnline = true;
      Serial.print(MQTTtopic);Serial.println(" Online");
    // ... and resubscribe
        
//    mqttclient.subscribe(calibration_target_topic.c_str());
   mqttclient.subscribe("home/WeeWX");
//    mqttclient.subscribe("home/#");
//    mqttclient.subscribe("home/garden/wind/#");
//    mqttclient.subscribe("home/env/#");
//    mqttclient.subscribe("home/temp/#");
//    mqttclient.subscribe("home/humidity/#");
//    mqttclient.subscribe("cmnd/HeatingSW/#");
   mqttclient.subscribe("tele/WXSTN/LWT/#");  //new for V036
//    mqttclient.subscribe("cmnd/HeatingSW/POWER");
//    mqttclient.subscribe("stat/HeatingSW/POWER");
    


  } else
  {
   Serial.print(F("failed, rc="));
   Serial.print(mqttclient.state());
   Serial.println(F(" try again later!"));
  }
return mqttclient.connected();
}
//Pring MQTT Stuff to Serial (used for debuging etc)
void MQTT_Debug(char* topic, String msgString, unsigned int length)
{ 
  Serial.print(F("Message arrived:  topic:" ));
  Serial.print(String(topic).substring(5));
  Serial.print(F(":  Length: ")); 
  Serial.print(String(length,DEC));
  Serial.print(F("  Payload: "));
  Serial.println(msgString);
}
// handles message arrived on subscribed topic(s)
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  char message_buff[700];
  int i = 0;

  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
    }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
   
if (MQTTdebug) { MQTT_Debug(topic, msgString, length);}

 
//Have we got temperature?  
   if (strcmp(topic,"home/garden/TempC")==0) {
 
    //TempC = msgString;
    //TempC = (float) WeeWX["outTemp"];
    WXStationOnlineTimer = millis();
    WXS_Online=true;
  };


//Have we got WXSTN LWT?  //new for V 036
   if (strcmp(topic,"tele/WXSTN/LWT")==0) {
                                            if (strcmp(message_buff,"Offline")==0) { WXS_Online=false; }
                                            if (strcmp(message_buff,"Online")==0) { WXStationOnlineTimer = millis(); WXS_Online=true; }
                                          };

if (strcmp(topic,"home/WeeWX")==0) {     
  //Serial.println("WeeWX MQTT received!");
  //Serial.println("!!!!!!!!!!!!!!!!!!! GOT WeeWX MQTT");
  if (acquireDataMutex(pdMS_TO_TICKS(100))) {
    auto error = deserializeJson(WeeWX, msgString);
    WeeWX_Valid = 1;
    TempC = WeeWX["outTemp"].as<float>();
    Humidity = WeeWX["outHumidity"].as<float>();
    Rain1h =  WeeWX["rainRate"].as<float>();
    Rain24h = WeeWX["rain24"].as<float>();
    DP =  WeeWX["dp"].as<float>();
    Pressure = WeeWX["barometer"].as<float>();
    Pressuretrend = WeeWX["barotrend"].as<String>();
    UV = WeeWX["cloud_cover"].as<int>();
    WXStationOnlineTimer = millis();
    WXS_Online=true;
    if (error) { Serial.println("Failed to parse WeeWX MQTT"); 
     // WeeWX_Valid = 0;
    }
    releaseDataMutex();
  }
}
}

void mqttclient_pub(String MQTTTOPIC, String MQTTMSG, boolean mqttdebug, boolean mqttpersistence)
{
  if (!mqttclient.connected()) {
      reconnect(); 
      }
  char MQTTmsg[MQTTMSG.length()+1];
  char MQTTtopic[50];
  MQTTMSG.toCharArray(MQTTmsg,MQTTMSG.length()+1); 
  MQTTTOPIC.toCharArray(MQTTtopic,50); 
  if(mqttclient.publish(MQTTtopic,MQTTmsg,mqttpersistence))  //do not publish retained message so all can see teh latest values
    {
    MQTTTOPIC += "=";
    MQTTTOPIC += MQTTMSG; 
        MqttCount++;
    } else
    {
    MQTTTOPIC += " failed to publish!";  
    }  
  
  
}

