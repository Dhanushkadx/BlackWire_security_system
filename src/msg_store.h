#ifndef OFFLINEMESSAGEHANDLER_H
#define OFFLINEMESSAGEHANDLER_H
#include "Arduino.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <freertos/queue.h>
// Constants
// Path to store messages
#define FILE_PATH ""
#define FILE_PREFIX "/messages_"
#define FILE_EXTENSION ".json"
#define MAX_FILE_SIZE 1024  // 4KB file size limit
#define FILE_INDEX_PATH "/file_index.txt"  // Path to store the current file index
#define JSON_DOC_SIZE 4096            // Adjust based on expected JSON message size
extern PubSubClient client;
//#include <queue.h>

extern xQueueHandle msgQueue;  // Declare a FreeRTOS queue to hold messages

// Define maximum size for the message (adjust as needed)
#define MAX_MSG_SIZE 512
// Function Declarations

/**
 * @brief Save a message to SPIFFS as part of a JSON array.
 * 
 * @param msg The JSON message string to be saved.
 */
void initMsgQueue();
void processMessagesFromQueue();
bool readLastNMessagesToQueue(int numMessages);

void saveMessageToSPIFFSV3(JsonDocument& msg);

uint8_t getNextFileIndex();

void saveFileIndex(int fileIndex);
/**
 * @brief Process and send offline messages from SPIFFS if the network is available.
 */
bool processOfflineMessagesV2();


/**
 * @brief Send a message over the network.
 * 
 * @param msg The JSON message to be sent.
 * @return true if the message was sent successfully, false otherwise.
 */
bool sendNetworkMessage(const String &msg);

#endif // OFFLINEMESSAGEHANDLER_H
