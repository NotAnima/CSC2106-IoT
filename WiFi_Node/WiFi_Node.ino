#include <M5StickCPlus.h>
#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

// Global UDP object
WiFiUDP udp;
const unsigned int udpPort = 4210; // UDP port for communication

// Replace with actual network credentials
const char* ssid = "10-100";
const char* password = "10100240";

struct RoutingTableEntry {
  String nodeID;
  String ip;
  String mac;
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

// Initializes the M5StickCPlus device and connects to the specified WiFi network.
// Prints the connection status and the device's IP address upon successful connection.
void setupWiFi() {
  M5.begin();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
Serial.print("Start WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting ..");
  }
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  // Initialize UDP
  udp.begin(udpPort);

}

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

// Updates the routing table with the given node ID, IP, and MAC address.
// If the node ID already exists in the table, updates its IP and MAC; otherwise, adds a new entry.
void updateRoutingTable(String nodeID, String ip, String mac) {
  if (nodeID != WiFi.macAddress()) { // Exclude device's own IP & MAC
    for (auto& entry : routingTable) {
      if (entry.nodeID == nodeID) {
        entry.ip = ip;
        entry.mac = mac;
        return;
      }
    }
    RoutingTableEntry newEntry = {nodeID, ip, mac};
    routingTable.push_back(newEntry);
  }
}

// Appends the given node ID to the node history vector, recording the sequence of nodes involved in packet transmission.
// This operation is crucial for tracking the path of data packets through the network, aiding in debugging and analysis.
// void updateNodeHistory(String nodeID) {
//   nodeHistory.push_back(nodeID);
// }

// // Constructs and sends a node packet containing information about the root sender, sender node, receiver node, and bin capacity.
// // The packet is serialized into a JSON string for transmission.
// void sendNodePacket(const NodePacket& packet) {
//   StaticJsonDocument<200> doc;
//   doc["rootSender"] = packet.rootSender;
//   doc["senderNode"] = packet.senderNode;
//   doc["receiverNode"] = packet.receiverNode;
//   doc["binCapacity"] = packet.binCapacity;
//   String output;
//   serializeJson(doc, output);

//   // TODO: Add code to send 'output' to the receiver node via WiFi

//   Serial.println("Sending Node Packet: " + output);
// }

void sendPing() {
  StaticJsonDocument<200> doc;
  doc["action"] = "ping";
  // Assuming a unique identifier for each M5StickC Plus device
  doc["senderNode"] = WiFi.macAddress(); 
  String output;
  serializeJson(doc, output);
  
  // Broadcast address for the subnet (modify according to your network configuration)
  IPAddress broadcastIp(192, 168, 50, 255); 

  udp.beginPacket(broadcastIp, udpPort);
  udp.write((const uint8_t *)output.c_str(), output.length());
  udp.endPacket();
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
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    
    String action = doc["action"];
    if (action == "ping") {
      StaticJsonDocument<200> ackDoc;
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
    } else if (action == "ack") {
      String responderIP = udp.remoteIP().toString();
      String responderMAC = doc["mac"];
      String nodeIDStr = doc["nodeID"]; // Using the MAC address string directly
      
      updateRoutingTable(nodeIDStr, responderIP, responderMAC);
    }
  }
}

void setup() {
  setupWiFi();
}

unsigned long lastDisplayUpdate = 0;
const long displayInterval = 5000; // Update the display every 5000 milliseconds (5 seconds)

void loop() {
  unsigned long currentMillis = millis();

  sendPing(); 
  receivePing();

  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = currentMillis;
    displayInfo(); // Combined display function
  }
}

