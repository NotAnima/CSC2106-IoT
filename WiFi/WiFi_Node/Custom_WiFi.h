#ifndef Custom_WiFi
#define Custom_WiFi

#undef min
#undef max

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <queue>

// Global UDP object
WiFiUDP udp;
const unsigned int udpPort = 4210; // UDP port for communication

// Replace with actual network credentials
const char* ssid = "ssid";
const char* password = "password";

unsigned long lastDisplayUpdate = 0;
const long displayInterval = 5000; // Update the display every 5000 milliseconds (5 seconds)
unsigned long lastRoutingTableCheck = 0; // Variable to track the last time routing table was checked
float binCapacity = 0.0;

// Time threshold for removing nodes from routing table (in milliseconds)
const unsigned long pingTimeout = 10000; // 10 seconds

// Global variable to keep track of hops
unsigned int hops = 0;

struct RoutingTableEntry {
  String nodeID;
  String ip;
  String mac;
  unsigned long timestamp; // Timestamp for the last received ping
};

struct NodePacket {
  String rootSender;    // Original sender of the packet
  String senderNode;    // Current sender of the packet
  String receiverNode;  // Next receiver of the packet
  float binCapacity;    // Capacity of the bin
  unsigned long rootTimestampSent; // Timestamp when the packet was originally sent
};

// A dynamic array of RoutingTableEntry objects, used to store and manage routing information.
// Each entry contains a node ID, its IP address, and MAC address.
// This vector allows for efficient addition and removal of routing entries as the network topology changes.
std::vector<RoutingTableEntry> routingTable;

// Update the queue to store NodePacket structures
std::queue<NodePacket> packetQueue;

// Function to add a NodePacket to the queue
void enqueuePacket(const NodePacket& packet) {
  packetQueue.push(packet);
}

// Function to process and remove the next NodePacket from the queue
void dequeueAndProcessPacket() {
  if (!packetQueue.empty()) {
    NodePacket packet = packetQueue.front();
    packetQueue.pop();
    // Example processing: Print the packet's details
    Serial.println("Processing packet from " + packet.rootSender);
    Serial.println("Bin Capacity: " + String(packet.binCapacity));
    // Further processing can be done here, such as forwarding the packet
  }
}

void printAndRequeuePackets() {
  std::queue<NodePacket> tempQueue;

  while (!packetQueue.empty()) {
    NodePacket packet = packetQueue.front();
    packetQueue.pop();
    Serial.println("Packet Details:");
    Serial.println(" Root Sender: " + packet.rootSender);
    Serial.println(" Sender Node: " + packet.senderNode);
    Serial.println(" Receiver Node: " + packet.receiverNode);
    Serial.println(" Bin Capacity: " + String(packet.binCapacity));
    Serial.println(" Timestamp Sent: " + String(packet.rootTimestampSent));
    tempQueue.push(packet);
  }

  // Move the packets back to the original queue
  while (!tempQueue.empty()) {
    packetQueue.push(tempQueue.front());
    tempQueue.pop();
  }
}

// TODO: Phileo can add in ultrasonic code in this
float getBinCapacity(){
  if(binCapacity >= 100)
  {
    binCapacity = binCapacity - 100;
  }
  binCapacity = binCapacity + 1;
  return binCapacity;
}

void getBinCapacityTask(void *parameter){
  for(;;){ // Infinite loop
    getBinCapacity();
    Serial.println("Bin Capacity of " + WiFi.macAddress() + ": " + String(binCapacity));
    vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for 1 minute (60000 milliseconds)
  }
}

// Function to display info
void displayInfo() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.print("MAC: ");
  M5.Lcd.println(WiFi.macAddress());

  M5.Lcd.println("Routing Table:");
  for (auto& entry : routingTable) {
    M5.Lcd.printf("ID: %d, IP: %s, MAC: %s\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str());
  }
}

// Function to display routing table on the Serial Monitor
void displayRoutingTableSerial() {
  Serial.println("Routing Table:");
  for (auto& entry : routingTable) {
    Serial.printf("ID: %d, IP: %s, MAC: %s\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str());
  }
}

// Updates the routing table with the given node ID, IP, MAC address, and timestamp.
// If the node ID already exists in the table, updates its IP, MAC, and timestamp; otherwise, adds a new entry.
void updateRoutingTable(String nodeID, String ip, String mac, unsigned long timestamp) {
  if (nodeID != WiFi.macAddress()) { // Exclude device's own IP & MAC
    for (auto& entry : routingTable) {
      if (entry.nodeID == nodeID) {
        entry.ip = ip;
        entry.mac = mac;
        entry.timestamp = timestamp;
        return;
      }
    }
    RoutingTableEntry newEntry = {nodeID, ip, mac, timestamp};
    routingTable.push_back(newEntry);
  }
}

void sendPing() {
  JsonDocument doc;
  doc["action"] = "ping";
  // Assuming a unique identifier for each M5StickC Plus device
  doc["senderNode"] = WiFi.macAddress();
  String output;
  serializeJson(doc, output);

  IPAddress broadcastIp = WiFi.gatewayIP(); // Get the gateway IP
  broadcastIp[3] = 255; // Convert to the broadcast IP

  udp.beginPacket(broadcastIp, udpPort);
  udp.write((const uint8_t *)output.c_str(), output.length());
  udp.endPacket();
}

// Removes nodes from the routing table where the time since the last ping exceeds the ping timeout threshold.
void removeInactiveNodes() {
  unsigned long currentMillis = millis();
  for (auto it = routingTable.begin(); it != routingTable.end();) {
    if (currentMillis - it->timestamp >= pingTimeout) {
      it = routingTable.erase(it);
    } else {
      ++it;
    }
  }
}

void receivePing() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = '\0';
    }
    String data(packetBuffer);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (!error) {
      String action = doc["action"];
      if (action == "ping") {
        String senderMAC = doc["senderNode"]; // Retrieve sender's MAC address from the packet
        IPAddress responderIP = udp.remoteIP();
        
        // // Commmented out to prevent broadcast storming
        // JsonDocument ackDoc;
        // ackDoc["action"] = "ack";
        // ackDoc["nodeID"] = WiFi.macAddress(); 
        // ackDoc["ip"] = WiFi.localIP().toString();
        // ackDoc["mac"] = WiFi.macAddress();
        // String ackMsg;
        // serializeJson(ackDoc, ackMsg);

        // udp.beginPacket(responderIP, udpPort);
        // udp.write((const uint8_t*)ackMsg.c_str(), ackMsg.length());
        // udp.endPacket();

        updateRoutingTable(senderMAC, responderIP.toString(), senderMAC, millis());
      }
    }
  }
}

void sendPacket() {
  if (routingTable.empty()) return; // Ensure there's at least one node in the routing table

  String serverMac = "4C:75:25:CB:89:5C"; // Server MAC address to look for
  String serverIp; // Variable to store server IP address

  // Find the server IP address based on its MAC
  for (const auto& entry : routingTable) {
    if (entry.mac.equalsIgnoreCase(serverMac)) {
      serverIp = entry.ip;
      break;
    }
  }

  // If server IP is not found, do not proceed
  if (serverIp.isEmpty()) {
    Serial.println("Server IP not found in routing table.");
    return;
  }

  binCapacity = getBinCapacity();
  NodePacket packet = {
    WiFi.localIP().toString(), // rootSender
    WiFi.localIP().toString(), // senderNode
    serverIp, // receiverNode - send to the server IP
    binCapacity, // binCapacity
    millis() // rootTimestampSent
  };

  JsonDocument doc;
  doc["action"] = "data";
  doc["rootSender"] = packet.rootSender;
  doc["senderNode"] = packet.senderNode;
  doc["receiverNode"] = packet.receiverNode;
  doc["binCapacity"] = packet.binCapacity;
  doc["rootTimestampSent"] = packet.rootTimestampSent;
  String output;
  serializeJson(doc, output);

  IPAddress receiverIp;
  receiverIp.fromString(packet.receiverNode);
  udp.beginPacket(receiverIp, udpPort);
  udp.write((const uint8_t *)output.c_str(), output.length());
  udp.endPacket();

  Serial.println("Packet Sent to Server: " + output);
}

void receivePacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, packetBuffer);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    if (doc.containsKey("action") && doc["action"].as<String>() == "ack") {
      // Handling ACK message
      Serial.println("ACK Received: " + String(packetBuffer));
      return; // Do not process further if it's an ACK message
    }

    Serial.println("Packet Received: " + String(packetBuffer));

    String serverMac = "4C:75:25:CB:89:5C"; // Server MAC address to look for
    String serverIp; // Variable to store server IP address

    // Find the server IP address based on its MAC
    for (const auto& entry : routingTable) {
      if (entry.mac.equalsIgnoreCase(serverMac)) {
        serverIp = entry.ip;
        break;
      }
    }

    // If server IP is not found, use an empty string (which should ideally never happen in this setup)
    if (serverIp.isEmpty()) {
      Serial.println("Server IP not found in routing table.");
      serverIp = String(""); // Use an empty string as fallback
    }

    NodePacket receivedPacket = {
      doc["rootSender"].as<String>(),
      WiFi.localIP().toString(), // Updating senderNode to current node
      serverIp, // Setting receiverNode to server's IP
      doc["binCapacity"].as<float>(),
      doc["rootTimestampSent"].as<unsigned long>()
    };
    enqueuePacket(receivedPacket);

    // Forward the updated packet if not reached the server
    sendPacket();
  }
}

#endif