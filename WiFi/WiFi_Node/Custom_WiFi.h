#ifndef Custom_WiFi
#define Custom_WiFi

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>

// Initialize Ultrasonice Sensor Pins
#include <HCSR04.h>
HCSR04 hc(0, 26);

#define MESH_PREFIX "dustbin"
#define MESH_PASSWORD "password"
#define MESH_PORT 5555
/*===================================================================*/
/*                     Custom Struct Declaration                     */
/*===================================================================*/
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

/*===================================================================*/
/*                         Function Prototypes                       */
/*===================================================================*/
void enqueueMessage(const CustomMessage& message);
float getActualBinCapacity();
float getBinCapacity();
void sendCustomMessage();
void displayLCD();
void getBinCapacityCallback();

// Task related comes after regular function prototypes
Task taskDisplayLCD(TASK_SECOND * 5, TASK_FOREVER, &displayLCD);
Task tSendCustomMessage(TASK_SECOND * 10, TASK_FOREVER, &sendCustomMessage);
Task taskGetBinCapacity(TASK_SECOND * 2, TASK_FOREVER, &getBinCapacityCallback);

/*===================================================================*/
/*                         Global variables                          */
/*===================================================================*/
Scheduler ts;
String latestReceivedMessage;
String latestSentMessage;
painlessMesh mesh;
// Declare the knownServers here first
std::vector<uint32_t> knownServers = {634095965};
 // will initially be the first server in the list
uint32_t preferredServer = knownServers[0];
std::vector<CustomMessage> messageQueue;
float binCapacity = 0.0;


/*===================================================================*/
/*                         Function Logics                           */
/*===================================================================*/
void enqueueMessage(const CustomMessage& message) {
    messageQueue.push_back(message);
    
    // Sort the vector based on priority, higher priority comes first
    std::sort(messageQueue.begin(), messageQueue.end(), [](const CustomMessage& a, const CustomMessage& b) {
        return a.priority > b.priority;
    });
}

// Mock Bin Capacity
float getBinCapacity() {
  if (binCapacity >= 100) {
    binCapacity = 0;
  }
  binCapacity += 1;
  return binCapacity;
}

// Actual Bin Capacity
float getActualBinCapacity(float distance){
  if(distance < 10){
    binCapacity = 100 - (distance*10);
  }else if(distance > 10){
    binCapacity=0;
  }
  else{
    binCapacity = 100;
  }
  return binCapacity;
}

void sendCustomMessage() {
  if (messageQueue.empty()) return;

  CustomMessage message = messageQueue.front();
  messageQueue.erase(messageQueue.begin());

  if (!mesh.sendSingle(message.targetId, message.payload)) {
      Serial.println("Failed to send message.");
      // reset the consecutive failed messages to 0 here
  } else {
      // Implement the algorithm to select a new known server and reset the consecutive failed msgs if(knownServers.size() > 2)
      Serial.println("Message sent successfully.");
  }

  latestSentMessage = "targetId: " + String(message.targetId) + ", payload: " + message.payload + ", priority: " + String(message.priority);
}

void displayLCD(){
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("NodeID: %u\n", mesh.getNodeId());
}

void getBinCapacityCallback() {
  float currentBinCapacity = getActualBinCapacity(hc.dist());
  // Comment bottom line and uncomment top line for ultrasonic data
  // float currentBinCapacity = getBinCapacity();

  Serial.println("Bin Capacity: " + String(currentBinCapacity));
  uint32_t targetId = preferredServer;

  StaticJsonDocument<200> doc;
  doc["rootSender"] = mesh.getNodeId();
  doc["binCapacity"] = currentBinCapacity;
  doc["rootTimestampSent"] = mesh.getNodeTime();

  String payload;
  serializeJson(doc, payload);

  CustomMessage message(targetId, payload, currentBinCapacity);
  enqueueMessage(message);
}


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
        return; // Early return if deserialization fails
    }

    // handles the broadcast to add new knownServers to the knownServers list
    if (inDoc["update"] == "update") {

        // handle the broadcast from the server to append to the list 
        // (deprecated because of ESP32 hardware issues: https://www.esp32.com/viewtopic.php?f=21&t=27265)
        // Keeping it here for proof of concept ideation
        
        knownServers.push_back(inDoc["newServer"].as<uint32_t>());
        for (int i = 0; i < knownServers.size(); i++) {
            Serial.println(knownServers.at(i));
        }
        // assign the updatedServer to be this newServer key
        preferredServer = inDoc["newServer"].as<uint32_t>();
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
