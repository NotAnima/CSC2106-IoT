#include <M5StickCPlus.h>
#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>

// Replace with actual network credentials
const char* ssid = "yourSSID";
const char* password = "yourPassword";

struct RoutingTableEntry {
  int nodeID;
  String ip;
  String mac;
};

struct NodePacket {
  int rootSender;
  int senderNode;
  int receiverNode;
  float binCapacity;
};

std::vector<RoutingTableEntry> routingTable;
std::vector<int> nodeHistory;

void setupWiFi() {
  M5.begin();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void updateRoutingTable(int nodeID, String ip, String mac) {
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

void updateNodeHistory(int nodeID) {
  nodeHistory.push_back(nodeID);
}

void sendNodePacket(const NodePacket& packet) {
  StaticJsonDocument<200> doc;
  doc["rootSender"] = packet.rootSender;
  doc["senderNode"] = packet.senderNode;
  doc["receiverNode"] = packet.receiverNode;
  doc["binCapacity"] = packet.binCapacity;
  String output;
  serializeJson(doc, output);

  // TODO: Add code to send 'output' to the receiver node via WiFi

  Serial.println("Sending Node Packet: " + output);
}

void setup() {
  setupWiFi();
  // Setup other initialisations if necessary
}

void loop() {
  // Example usage, replace with your own logic
  NodePacket examplePacket = {1, 2, 3, 0.9};
  sendNodePacket(examplePacket);
  delay(10000); // Delay for demonstration purposes, adjust as needed
}
