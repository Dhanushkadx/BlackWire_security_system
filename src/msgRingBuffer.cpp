#include "msgRingBuffer.h"
#include <string.h>

QueueHandle_t smsQueue = NULL;

void initSMSQueuex() {
    smsQueue = xQueueCreate(SMS_QUEUE_LENGTH, sizeof(SMS_t));
    if (smsQueue == NULL) {
    Serial.println("Failed to create SMS queue!");
}
    else {
        Serial.println("SMS queue created successfully.");
    }
}

void addSMS(const char* message, uint8_t type, const char* number) {
    // if (number == NULL) {
    //     Serial.println(F("Invalid SMS input: NULL number!"));
    //     return;
    // }
    // if(message == NULL) {
    //     Serial.println(F("Invalid SMS input: NULL message!"));
    //     return;

    // }

    SMS_t sms;
    memset(&sms, 0, sizeof(SMS_t));  // Clear structure to avoid garbage
    

    // Safe copy for number (up to 19 chars + null)
    if(number!=NULL){strncpy(sms.number, number, sizeof(sms.number) - 1);}else{ strcpy(sms.number, "0");} // Ensure null termination

    // Safe copy for message (up to 159 chars + null)
    if(message!=NULL){strncpy(sms.message, message, sizeof(sms.message) - 1);}else{ strcpy(sms.message, "0");} // Ensure null termination

    sms.type = type;

    // Add to queue or overwrite oldest if full
    if (xQueueSend(smsQueue, &sms, 0) != pdPASS) {
        SMS_t temp;
        xQueueReceive(smsQueue, &temp, 0);  // Remove oldest
        xQueueSend(smsQueue, &sms, 0);
    }
}


bool getNextSMS(SMS_t* smsOut) {
    return (xQueueReceive(smsQueue, smsOut, 0) == pdPASS);
}
