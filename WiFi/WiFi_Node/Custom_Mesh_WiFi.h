#ifndef Custom_Mesh_WiFi
#define Custom_Mesh_WiFi

#include <painlessMesh.h>

// Network credentials
#define   MESH_PREFIX     "painlessMesh"
#define   MESH_PASSWORD   "meshPassword"
#define   MESH_PORT       5555

painlessMesh  mesh;

// Forward declarations
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();

std::queue<String> messageQueue;

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received message from %u: %s\n", from, msg.c_str());
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Msg from %u: %s\n", from, msg.c_str());

  // Optionally, enqueue the received message for further processing or forwarding
  // enqueueMessage("Forward: " + msg); // Uncomment if you want to forward or process the message further
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  M5.Lcd.printf("New Conn, ID = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  M5.Lcd.printf("Conn changed\n");
}

void enqueueMessage(String msg) {
  messageQueue.push(msg);
}

#endif
