#ifndef SMS_BUFFER_H
#define SMS_BUFFER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "Arduino.h"

#define SMS_QUEUE_LENGTH 10

typedef struct {
    char number[20];
    char message[160];
    uint8_t type; // 0: normal, 1: alarm, 2: info, etc.
} SMS_t;

extern QueueHandle_t smsQueue;

void initSMSQueuex();
void addSMS(const char* message, uint8_t type, const char* number);
bool getNextSMS(SMS_t* smsOut);

#endif
