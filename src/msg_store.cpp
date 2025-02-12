#include "msg_store.h"

xQueueHandle msgQueue;
// Function to save message
void saveMessageToSPIFFSV3(JsonDocument& msg) {
    uint32_t file_size;
    // Initialize SPIFFS if not already initialized
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    // Get the next available file index
    int fileIndex = getNextFileIndex();
    String currentFileName = FILE_PREFIX + String(fileIndex) + FILE_EXTENSION;

    // Check if the current file exists and its size
    File file = SPIFFS.open(currentFileName, FILE_READ);
    String jsonString = "[]"; // Default empty array in case the file is empty
    if (file) {
        jsonString = file.readString();  // Read existing messages from the file
        file_size = file.size();
        file.close();
    }

    DynamicJsonDocument doc(2096); // Adjust size as needed
    deserializeJson(doc, jsonString);

    // Create or get the "msg" array
    JsonArray array = doc["msg"];
    if (!array) {
        // If "msg" array doesn't exist, create one
        array = doc.createNestedArray("msg");
    }

    // Add the received message (`msg`) as a new object inside the "msg" array
    JsonObject newMsg = array.createNestedObject();

    // Copy all fields from the passed `msg` (JsonDocument) into the new object
    for (JsonPair keyValue : msg.as<JsonObject>()) {
        newMsg[keyValue.key()] = keyValue.value(); // Copy each key-value pair
    }

    // Save the updated JSON document back to SPIFFS (in the new file if necessary)
    file = SPIFFS.open(currentFileName, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    
    serializeJson(doc, file);  // Write the updated JSON to the file
    file.close();

    // Check if the current file size exceeds 4KB
    if (file_size > MAX_FILE_SIZE) {
        // If file size exceeds 4KB, increment the file index and create a new file
        fileIndex++;
        Serial.println("go to next file");
        saveFileIndex(fileIndex);  // Save the new file index
        //currentFileName = FILE_PREFIX + String(fileIndex) + FILE_EXTENSION;
    }

    Serial.println("Message saved to SPIFFS: " + currentFileName);
}


bool sendNetworkMessage(const String &msg, char* topic){
    
    Serial.printf_P(PSTR("send stored msg:%s"),msg.c_str());
    if ( client.publish(topic, msg.c_str())) {
        Serial.println(F("ok"));
        return false;
    } else {
        Serial.println(F("Failed"));
        return true;
    } 
    
}

// Function to get the next available file index
uint8_t getNextFileIndex() {
    int fileIndex = 1;
    File file = SPIFFS.open(FILE_INDEX_PATH, FILE_READ);
    
    if (file) {
        String indexStr = file.readString();
        file.close();
        if (indexStr.length() > 0) {
            fileIndex = indexStr.toInt();  // Read the last file index
        }
    }
    
    return fileIndex;
}

// Function to save the updated file index to SPIFFS
void saveFileIndex(int fileIndex) {
    File file = SPIFFS.open(FILE_INDEX_PATH, FILE_WRITE);
    
    if (file) {
        file.print(fileIndex);  // Save the new file index
        file.close();
    }
}

// Function to process offline messages from multiple files
bool processOfflineMessagesV2() {
    if (!SPIFFS.begin(true)) {
        Serial.println(F("Failed to mount SPIFFS"));
        return true;
    }

    int fileIndex = 1; // Start from messages_1.json
    bool processed_result = true;

    while (true) {
        String currentFileName = FILE_PREFIX + String(fileIndex) + FILE_EXTENSION;
        // Check if the file exists
        if (!SPIFFS.exists(currentFileName)) {
            Serial.println(F("No more files to process."));
            // After processing all files, reset fileIndex to 1 and save it back
            saveFileIndex(1);  // Save the new file index
            processed_result = false;
            break;  // No more files, exit the loop
        }

        File file = SPIFFS.open(currentFileName, FILE_READ);
        if (!file) {
            Serial.println(F("Failed to open file for reading"));
            break;
        }
          // Check if the file is empty
        if (file.size() == 0) {
            Serial.println(F("File is empty, skipping..."));
            file.close();  // Close the file
            fileIndex++;   // Skip to the next file
            continue;      // Skip processing the current empty file
        }

        String jsonString = file.readString();
        Serial.printf_P(PSTR("MSG_HDLE-FILE %d CONT: %s \n"),fileIndex,jsonString.c_str());
       
        file.close();  // Close the file after reading

        DynamicJsonDocument doc(4096); // Adjust size as needed
        DeserializationError error = deserializeJson(doc, jsonString);
        if (error) {
            Serial.println(F("Failed to parse saved messages"));
            continue;  // Skip this file and move to the next one
        }

        JsonArray stored_msgs = doc["msg"];

        Serial.println(F("Processing offline messages"));
        // Iterate through each message and try to send it
        for (JsonArray::iterator it = stored_msgs.begin(); it != stored_msgs.end(); ++it) {
            JsonObject msg = (*it).as<JsonObject>();
            String serializedMsg;
            serializeJson(msg, serializedMsg);

            Serial.printf_P(PSTR("MSG_HDLE-NOW SEND: %s \n"),serializedMsg.c_str());
            Serial.println(serializedMsg);

            // Send each message over the network
            if (!sendNetworkMessage(serializedMsg)) {  // Assumes sendNetworkMessage() function is available
                // If sending ok, remove the message from the array
                stored_msgs.remove(it);
            } else {
                Serial.println(F("MSG_HDLE- Sending Faild Processing Faild!!")); 
                return true;
                processed_result = true;
                break; // Stop if network is unstable
            }
        }

        // Save updated messages back to the current file
        file = SPIFFS.open(currentFileName, FILE_WRITE);
        if (!file) {
            Serial.println(F("Failed to open file for writing"));
            return true;
        }
        serializeJson(doc, file);
        file.close();

        // Move to the next file if there are more files to process
        fileIndex++;
        
    }

    return processed_result;
}

bool readLastNMessagesToQueue(int numMessages) {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }

    // Start with the most recent file
    int fileIndex = getNextFileIndex();
    int messagesRead = 0;
    bool result = true;

    while (messagesRead < numMessages) {
        Serial.println("loop");
        String currentFileName = FILE_PREFIX + String(fileIndex) + FILE_EXTENSION;

        if (!SPIFFS.exists(currentFileName)) {
            Serial.println("No more files to process.");
            break;  // No more files, exit the loop
        }

        File file = SPIFFS.open(currentFileName, FILE_READ);
        if (!file) {
            Serial.println("Failed to open file for reading");
            result = false;
            break;
        }

        String jsonString = file.readString();
        file.close();  // Close the file after reading

        // Check if the file is empty
        if (jsonString.isEmpty()) {
            Serial.printf("File %s is empty, going to the previous file...\n", currentFileName);
            fileIndex--;  // Go to the previous file
            continue;  // Skip processing this empty file
        }

        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, jsonString);
        if (error) {
            Serial.println("Failed to parse saved messages");
            continue;  // Skip this file and move to the next one
        }

        JsonArray stored_msgs = doc["msg"];
        int numStoredMsgs = stored_msgs.size();

        // Process the last `numMessages` messages from this file
        for (int i = numStoredMsgs - 1; i >= 0 && messagesRead < numMessages; --i) {
            JsonObject msg = stored_msgs[i].as<JsonObject>();
            String serializedMsg;
            serializeJson(msg, serializedMsg);

            // Enqueue the message to FreeRTOS queue
            if (xQueueSend(msgQueue, serializedMsg.c_str(), portMAX_DELAY) != pdPASS) {
                Serial.println("Failed to enqueue message");
                result = false;
                break;
            }

            messagesRead++;  // Increment the number of messages read
        }

        // Stop if the required number of messages has been read
        if (messagesRead >= numMessages) {
            break;
        }

        // Move to the previous file
        fileIndex--;  
        if (fileIndex < 1) {
            Serial.println("Reached the first file. No more messages.");
            break;  // Reached the first file, stop
        }
    }

    return result;
}


// Function to process messages from the FreeRTOS queue (this can be a task)
void processMessagesFromQueue() {
     //creat topic
    uint8_t mac[6];
    char device_id_macStr[18];
    WiFi.macAddress(mac);	
  //Format the MAC address without colons and with underscores
    sprintf(device_id_macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
   
    char topic[50];
    memset(topic,'\0',50);
    uint16_t msg_no = 0;
    while (true) {
        char msg[MAX_MSG_SIZE];
        // Wait for a message with a finite timeout (e.g., 1000 ticks)
        if (xQueueReceive(msgQueue, msg, 1000 / portTICK_PERIOD_MS) == pdPASS) {
            // Process the message (e.g., send over the network or do something else)
            Serial.printf_P(PSTR("Processing message %d: %s\n"), msg_no, msg);
            sprintf_P(topic,PSTR("blackwire/%s/log/msg%d"),device_id_macStr,msg_no);
            sendNetworkMessage(String(msg),topic);
            msg_no++;
        }
        else{
            break;
        }
    }
}

// Function to initialize the FreeRTOS queue
void initMsgQueue() {
    msgQueue = xQueueCreate(10, MAX_MSG_SIZE);  // Create a queue that can hold 10 messages of size MAX_MSG_SIZE
    if (msgQueue == NULL) {
        Serial.println("Failed to create queue");
    } else {
        Serial.println("Queue created successfully");
    }
}

