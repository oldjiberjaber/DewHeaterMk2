#ifndef AAAA_JSON_H
#define AAAA_JSON_H

#include <ArduinoJson.h>

// Weather & Channel Telemetry JSON Documents
bool WeeWX_Valid = 0;
DynamicJsonDocument WeeWX(512);

DynamicJsonDocument jsonH0(512);
DynamicJsonDocument jsonH1(512);

#endif // AAAA_JSON_H