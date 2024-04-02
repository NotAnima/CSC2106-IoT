#ifndef Custom_WiFi
#define Custom_WiFi

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

#define MESH_PREFIX "dustbin"
#define MESH_PASSWORD "password"
#define MESH_PORT 5555

Scheduler ts;
String latestReceivedMessage;
String latestSentMessage;
painlessMesh mesh;

// Declare the knownServers here first
std::vector<uint32_t> knownServers = {634095965};
uint32_t preferredServer = knownServers[0]; // will initially be the first server in the list

float binCapacity = 0.0;

struct CustomMessage {
    uint32_t targetId;
    String payload;
    int priority;

    // Method to calculate priority based on bin capacity
    static int calculatePriority(float binCapacity) {
        if (binCapacity <= 25) return 1;
        else if (binCapacity <= 50) return 2;
        else if (binCapacity <= 75) return 3;
        else return 4;
    }

    // Constructor for easy creation of CustomMessage objects
    CustomMessage(uint32_t id, const String& pl, float binCap) : targetId(id), payload(pl), priority(calculatePriority(binCap)) {}
};

std::vector<CustomMessage> messageQueue;

void enqueueMessage(const CustomMessage& message) {
    messageQueue.push_back(message);
    
    // Sort the vector based on priority, higher priority comes first
    std::sort(messageQueue.begin(), messageQueue.end(), [](const CustomMessage& a, const CustomMessage& b) {
        return a.priority > b.priority;
    });
}

// TODO: Phileo to add in your ultrasonic code
float getBinCapacity() {
  if (binCapacity >= 100) {
    binCapacity = 0;
  }
  binCapacity += 1;
  return binCapacity;
}

void sendCustomMessage() {
  if (messageQueue.empty()) return;

  CustomMessage message = messageQueue.front();
  messageQueue.erase(messageQueue.begin());

  if (!mesh.sendSingle(message.targetId, message.payload)) {
      Serial.println("Failed to send message.");
  } else {
      Serial.println("Message sent successfully.");
  }

  latestSentMessage = "targetId: " + String(message.targetId) + ", payload: " + message.payload + ", priority: " + String(message.priority);
}

void displayLCD(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  M5.lcd.setRotation(3);
  M5.Lcd.printf("NodeID: %u\n", mesh.getNodeId());
  if(preferredServer == 634095965)
  {
    M5.Lcd.println("Grabbed the correct server ID");
  }
  // M5.Lcd.println("Preferred Server: " + preferredServer);
  // Serial.println("Preferred Server: " + preferredServer);
}

Task taskDisplayLCD(TASK_SECOND * 5, TASK_FOREVER, &displayLCD);

Task tSendCustomMessage(TASK_SECOND * 10, TASK_FOREVER, &sendCustomMessage);

void getBinCapacityCallback() {
    float currentBinCapacity = getBinCapacity();
    Serial.println("Bin Capacity: " + String(currentBinCapacity));

  uint32_t targetId = preferredServer;
  // uint32_t targetId = 634094909; //131

  StaticJsonDocument<200> doc;
  doc["rootSender"] = mesh.getNodeId();
  doc["binCapacity"] = currentBinCapacity;
  doc["rootTimestampSent"] = mesh.getNodeTime();

  String payload;
  serializeJson(doc, payload);

  CustomMessage message(targetId, payload, currentBinCapacity);
  enqueueMessage(message);
}

Task taskGetBinCapacity(TASK_SECOND * 10, TASK_FOREVER, &getBinCapacityCallback);

void onNewConnectionCallback(uint32_t nodeId) {
    // Serial.printf("New Connection: %u\n", nodeId);
}

void onDroppedConnectionCallback(uint32_t nodeId) {
    // Serial.printf("Dropped Connection: %u\n", nodeId);
}

void receivedCallback(uint32_t from, String &msg) {
    StaticJsonDocument<600> inDoc;
    DeserializationError error = deserializeJson(inDoc, msg);

    if (error) {
        // Serial.print(F("deserializeJson() failed with code "));
        // Serial.println(error.c_str());
        return; // Early return if deserialization fails
    }
    // handles the broadcast to add new knownServers to the knownServers list
    if (inDoc["update"] == "update") {
        // handle the broadcast from the server to append to the 
        knownServers.push_back(inDoc["newServer"].as<uint32_t>());
        // Check if appended
        for (int i = 0; i < knownServers.size(); i++) {
            Serial.println(knownServers.at(i));
        }
        return;
    }
    // Extract information directly from inDoc
    uint32_t originalID = inDoc["rootSender"];
    float binCapacity = inDoc["binCapacity"];
    unsigned long rootTimestampSent = inDoc["rootTimestampSent"];
    // Calculate priority based on the bin capacity
    int priority = CustomMessage::calculatePriority(binCapacity);

    latestReceivedMessage = "originalID: " + String(originalID) + ", binCapacity: " + String(binCapacity, 2) + ", timestamp: " + String(rootTimestampSent) + ", priority: " + String(priority);

    Serial.println("Latest Received Message: " + latestReceivedMessage);

    // Update display accordingly
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("NodeID: %u\n", mesh.getNodeId());
    M5.Lcd.printf("Capacity: %d%%\nSent: %s\nRecv: %s", int(binCapacity), latestSentMessage.c_str(), latestReceivedMessage.c_str());
}

#endif
