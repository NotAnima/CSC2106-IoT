#ifndef Custom_WiFi
#define Custom_WiFi

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

// Global UDP object
WiFiUDP udp;
const unsigned int udpPort = 4210; // UDP port for communication

// Replace with actual network credentials
const char* ssid = "wifi_name";
const char* password = "wifipassword";

unsigned long lastDisplayUpdate = 0;
const long displayInterval = 5000; // Update the display every 5000 milliseconds (5 seconds)
unsigned long lastRoutingTableCheck = 0; // Variable to track the last time routing table was checked

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

// A dynamic array of integers, representing the sequence of node IDs involved in packet transmission.
// This vector is used to track the path of data packets through the network, facilitating debugging and analysis.
std::vector<int> nodeHistory;

// Function to display info
void displayInfo() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  M5.Lcd.println("Routing Table:");
  for (auto& entry : routingTable) {
    M5.Lcd.printf("ID: %d, IP: %s, MAC: %s\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str());
  }

  M5.Lcd.println("Node History:");
  for (int nodeId : nodeHistory) {
    M5.Lcd.printf("Node ID: %d\n", nodeId);
  }
}

// Function to display routing table on the Serial Monitor
void displayRoutingTableSerial() {
  Serial.println("Routing Table:");
  for (auto& entry : routingTable) {
    Serial.printf("ID: %d, IP: %s, MAC: %s\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str());
  }
}

// Function to display node history on the Serial Monitor
void displayNodeHistorySerial() {
  Serial.println("Node History:");
  for (int nodeId : nodeHistory) {
    Serial.printf("Node ID: %d\n", nodeId);
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
        entry.timestamp = timestamp; // Update timestamp
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
  Serial.println(broadcastIp);
  // Broadcast address for the subnet
  // use the gateway ip later 
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
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    
    String action = doc["action"];
    if (action == "ping") {
      JsonDocument ackDoc;
      ackDoc["action"] = "ack";
      ackDoc["nodeID"] = WiFi.macAddress(); // Using MAC address directly
      ackDoc["ip"] = WiFi.localIP().toString();
      ackDoc["mac"] = WiFi.macAddress();
      String ackMsg;
      serializeJson(ackDoc, ackMsg);

      IPAddress responderIP = udp.remoteIP();
      udp.beginPacket(responderIP, udpPort);
      udp.write((const uint8_t*)ackMsg.c_str(), ackMsg.length());
      udp.endPacket();

      // Update or add entry in routing table with current timestamp
      updateRoutingTable(doc["senderNode"].as<String>(), responderIP.toString(), WiFi.macAddress(), millis());
    }
  }
}

void sendAck(const String& originalSender) {
  JsonDocument ackDoc;
  ackDoc["action"] = "ack";
  ackDoc["to"] = originalSender;
  ackDoc["from"] = WiFi.localIP();
  ackDoc["message"] = "Packet received at server";
  String ackMsg;
  serializeJson(ackDoc, ackMsg);

  IPAddress senderIp;
  // Assuming originalSender can be directly converted to IP, in real scenarios, a lookup would be required
  senderIp.fromString(originalSender);
  udp.beginPacket(senderIp, udpPort);
  udp.write((const uint8_t *)ackMsg.c_str(), ackMsg.length());
  udp.endPacket();

  Serial.println("ACK Sent: " + ackMsg);
}

void sendPacket() {
  if (routingTable.empty()) return; // Ensure there's at least one node in the routing table

  NodePacket packet = {
    WiFi.localIP().toString(), // rootSender
    WiFi.localIP().toString(), // senderNode
    routingTable[0].ip, // receiverNode - sending to the first node in the routing table
    0.75, // binCapacity - example capacity, replace with actual sensor data
    millis() // rootTimestampSent
  };

  JsonDocument doc;
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

  Serial.println("Packet Sent: " + output);
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
    hops++;

    if (hops >= 2) {
      // Assuming packet reached the server after 2 hops
      String originalSender = doc["rootSender"].as<String>();
      sendAck(originalSender);
      hops = 0; // Reset hops for next message
    } else {
      NodePacket receivedPacket = {
        doc["rootSender"].as<String>(),
        WiFi.localIP().toString(), // Updating senderNode to current node
        routingTable.empty() ? String("") : routingTable[0].ip, // Setting receiverNode to first node in the routing table
        doc["binCapacity"].as<float>(),
        doc["rootTimestampSent"].as<unsigned long>()
      };

      // Forward the updated packet if not reached the server
      sendPacket();
    }
  }
}

void timedSendPacket() {
  static unsigned long lastSendTime = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= 10000) { // 10 seconds
    lastSendTime = currentMillis;
    sendPacket();
  }
}



#endif