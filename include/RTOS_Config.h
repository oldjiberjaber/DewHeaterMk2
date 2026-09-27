#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Global Data Mutex for protecting shared states across tasks
extern SemaphoreHandle_t xDataMutex;

// Global flag to indicate OTA update in progress
extern volatile bool isOTAInProgress;

// Helper function to acquire data mutex safely
inline bool acquireDataMutex(TickType_t waitTicks = portMAX_DELAY) {
  if (xDataMutex != NULL) {
    return (xSemaphoreTake(xDataMutex, waitTicks) == pdTRUE);
  }
  return true;
}

// Helper function to release data mutex
inline void releaseDataMutex() {
  if (xDataMutex != NULL) {
    xSemaphoreGive(xDataMutex);
  }
}

