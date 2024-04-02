#ifndef Custom_WiFi
#define Custom_WiFi

#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <queue>

#define MESH_PREFIX "meshNetwork"
#define MESH_PASSWORD "meshPassword"
#define MESH_PORT 5555

Scheduler ts;
String latestReceivedMessage;
String latestSentMessage;
painlessMesh mesh;

float binCapacity = 0.0;

struct NodePacket {
  String rootSender;    // Original sender of the packet
  float binCapacity;    // Capacity of the bin
  unsigned long rootTimestampSent; // Timestamp when the packet was originally sent
};

struct CustomMessage {
  uint32_t targetId;
  String payload;
  int priority;
};

std::queue<NodePacket> packetQueue;

void enqueuePacket(const NodePacket& packet) {
  packetQueue.push(packet);
}

void processPacketQueue() {
  while (!packetQueue.empty()) {
    NodePacket packet = packetQueue.front();
    packetQueue.pop();
    Serial.println("Processing packet from " + packet.rootSender);
    Serial.println("Bin Capacity: " + String(packet.binCapacity));
  }
}

Task taskProcessPacketQueue(TASK_SECOND * 1, TASK_FOREVER, &processPacketQueue);

float getBinCapacity() {
  if (binCapacity >= 100) {
    binCapacity = 0;
  }
  binCapacity += 1;
  return binCapacity;
}

// In Custom_WiFi.h or similar, adjust the getBinCapacityCallback function
void getBinCapacityCallback() {
  binCapacity = getBinCapacity();
  Serial.println("Bin Capacity: " + String(binCapacity));
}

void sendCustomMessage() {
  uint32_t targetId = 2510821181; //36
  // uint32_t targetId = 634094909; //131
  int priority = 4;

  StaticJsonDocument<200> doc;
  doc["rootSender"] = mesh.getNodeId();
  doc["binCapacity"] = getBinCapacity();
  doc["rootTimestampSent"] = millis();
  String payload;
  serializeJson(doc, payload);

  if (!mesh.sendSingle(targetId, payload)) {
    Serial.println("Failed to send message.");
  } else {
    Serial.println("Message sent successfully.");
  }

  StaticJsonDocument<300> outDoc;
  outDoc["targetId"] = targetId;
  outDoc["payload"] = payload;
  outDoc["priority"] = priority;
  String outMessage;
  serializeJson(outDoc, outMessage);

  mesh.sendSingle(targetId, outMessage);

  latestSentMessage = "targetId: " + String(targetId) + ", payload: " + payload + ", priority: " + String(priority);
}

Task taskGetBinCapacity(TASK_SECOND * 10, TASK_FOREVER, &getBinCapacityCallback);
Task tSendCustomMessage(TASK_SECOND * 10, TASK_FOREVER, &sendCustomMessage);

void onNewConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection: %u\n", nodeId);
}

void onDroppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Dropped Connection: %u\n", nodeId);
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<300> inDoc;
  deserializeJson(inDoc, msg);
  uint32_t originalID = inDoc["targetId"];
  String payload = inDoc["payload"];
  int priority = inDoc["priority"];

  latestReceivedMessage = "originalID: " + String(originalID, DEC) + ", payload: " + payload + ", priority: " + String(priority);

  Serial.println("Current Node Bin Capacity: " + String(getBinCapacity()) + "%");
  Serial.println("Latest Sent Message: " + latestSentMessage);
  Serial.println("Latest Received Message: " + latestReceivedMessage);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("NodeID: %u", mesh.getNodeId());
  M5.Lcd.printf("Cap: %d%%\nSent: %s\nRecv: %s", int(getBinCapacity()), latestSentMessage.c_str(), latestReceivedMessage.c_str());
}

#endif
