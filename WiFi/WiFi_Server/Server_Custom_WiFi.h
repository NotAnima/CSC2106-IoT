#ifndef Custom_WiFi
#define Custom_WiFi

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>
#include <WiFiUdp.h>

void sendAck(const String& originalSender);
// Global UDP object
WiFiUDP udp;
const unsigned int udpPort = 4210; // UDP port for communication

// Replace with actual network credentials
const char* ssid = "Poh";
const char* password = "lsps353ycss";

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
  boolean isOn;
  unsigned long timestamp; // Timestamp for the last received ping
};


struct NodePacket {
  String rootSender;    // Original sender of the packet
  String senderNode;    // Current sender of the packet
  String receiverNode;  // Next receiver of the packet
  float binCapacity;    // Capacity of the bin
  unsigned long rootTimestampSent; // Timestamp when the packet was originally sent
};

struct dustbin_info{
  String rootIP;
  String binInfo;
};
// A dynamic array of RoutingTableEntry objects, used to store and manage routing information.
// Each entry contains a node ID, its IP address, and MAC address.
// This vector allows for efficient addition and removal of routing entries as the network topology changes.
std::vector<RoutingTableEntry> routingTable;

// A dynamic array of integers, representing the sequence of node IDs involved in packet transmission.
// This vector is used to track the path of data packets through the network, facilitating debugging and analysis.
std::vector<dustbin_info> nodeHistory;

float getBinCapacity(){
  if(binCapacity >= 100)
  {
    binCapacity = binCapacity - 100;
  }
  binCapacity = binCapacity + 1;
  return binCapacity;
}

// Function to display info
void displayInfo() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  M5.Lcd.println("RT:");
  for (auto& entry : routingTable) {
    M5.Lcd.printf("IP: %s isOn: %s\n", entry.ip.c_str(), entry.isOn? "True":"False");
  }

  M5.Lcd.println("Node History:");
  int startIndex = (nodeHistory.size() > 5) ? nodeHistory.size() - 5 : 0;
  for (int i = startIndex; i < nodeHistory.size(); i++) {
    M5.Lcd.printf("%s: %s\%\n", nodeHistory[i].rootIP.c_str(), nodeHistory[i].binInfo.c_str());
  }
}

// Function to display routing table on the Serial Monitor
void displayRoutingTableSerial() {
  Serial.println("Routing Table:");
  for (auto& entry : routingTable) {
    Serial.printf("ID: %d, IP: %s, MAC: %s, isOn: %d\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str(), entry.isOn);
  }
}

// Function to display node history on the Serial Monitor
void displayNodeHistorySerial() {
  Serial.println("Node History:");
  for (auto &info : nodeHistory) {
    Serial.printf("%s: %s\%\n", info.rootIP.c_str(), info.binInfo.c_str());
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
        entry.isOn = true;
        entry.timestamp = timestamp;
        return;
      }
    }
    RoutingTableEntry newEntry = {nodeID, ip, mac, true, timestamp};
    routingTable.push_back(newEntry);
  }
}

void updateHistory(String ipAddress, String binInfo) {
    dustbin_info newInfo = {ipAddress, binInfo};
    nodeHistory.push_back(newInfo);
}


void sendPing() {
  JsonDocument doc;
  doc["action"] = "ping";
  // Assuming a unique identifier for each node
  doc["senderNode"] = WiFi.macAddress(); 
  doc["server"] = true;
  String output;
  serializeJson(doc, output);

  IPAddress broadcastIp = WiFi.gatewayIP(); // Get the gateway IP
  broadcastIp[3] = 255; // Convert to the broadcast IP

  udp.beginPacket(broadcastIp, udpPort);
  udp.write((const uint8_t *)output.c_str(), output.length());
  udp.endPacket();
}


void markInactiveNodesAsOff() {
  unsigned long currentMillis = millis();
  for (auto& entry : routingTable) {
      if ( (currentMillis - entry.timestamp) >= pingTimeout) {
        entry.isOn = false;
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

    // handle incorrectly parsed json data
    if (error) {
      return;
    }
    
    String action = doc["action"];
    if (action == "ping") {
      // handle updating of the routing table
      IPAddress responderIP = udp.remoteIP();
      updateRoutingTable(doc["senderNode"].as<String>(), udp.remoteIP().toString(), WiFi.macAddress(), millis());
    }
    else if (doc.containsKey("action") && doc["action"].as<String>() == "data") {
      // Handling ACK message
      String originalSender = doc["rootSender"].as<String>();
      displayInfo();
      float binData = doc["binCapacity"].as<float>();
      Serial.printf("Dustbin data on: %s is about %.2f percent full now\n", doc["rootSender"].as<String>(), binData);
      sendAck(originalSender);

      // convert the bindata into a 2 decimal string
      String binInfo = doc["rootTimestampSent"].as<String>() + "@ " + String(binData, 2);
                        /* If possible change to AM/PM using NTP */
      updateHistory(doc["rootSender"].as<String>(), binInfo);
      return;
    }
    else if (doc.containsKey("action") && doc["action"].as<String>() == "ack"){
      // do nothing if ack
      Serial.println("ACK received, dropping packet");
      return;
    }
  }
}

void sendAck(const String& originalSender) {
  JsonDocument ackDoc;
  ackDoc["action"] = "ack";
  ackDoc["to"] = originalSender;
  ackDoc["from"] = WiFi.localIP();
  ackDoc["message"] = "Packet received at server";
  ackDoc["server"] = true;
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



#endif