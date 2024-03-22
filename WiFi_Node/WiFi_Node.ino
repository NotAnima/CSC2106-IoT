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
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Updates the routing table with the given node ID, IP, and MAC address.
// If the node ID already exists in the table, updates its IP and MAC; otherwise, adds a new entry.
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

// Appends the given node ID to the node history vector, recording the sequence of nodes involved in packet transmission.
// This operation is crucial for tracking the path of data packets through the network, aiding in debugging and analysis.
void updateNodeHistory(int nodeID) {
  nodeHistory.push_back(nodeID);
}

// Constructs and sends a node packet containing information about the root sender, sender node, receiver node, and bin capacity.
// The packet is serialized into a JSON string for transmission.
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

// Example usage of sending a node packet.
// This demonstrates how to create and send a packet with example data.
void loop() {
  // Example usage, replace with your own logic
  NodePacket examplePacket = {1, 2, 3, 0.9};
  sendNodePacket(examplePacket);
  delay(10000); // Delay for demonstration purposes, adjust as needed
}
