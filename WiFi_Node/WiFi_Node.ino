#include <M5StickCPlus.h>
#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>

// Replace with actual network credentials
const char* ssid = "AT_13";
const char* password = "connectiongranted";

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
  long rootTimestampSent;
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
}

// Function to display routing table on the M5StickC Plus LCD
void displayRoutingTable() {
  M5.Lcd.println("Routing Table:");
  for (auto& entry : routingTable) {
    M5.Lcd.printf("ID: %d, IP: %s, MAC: %s\n", entry.nodeID, entry.ip.c_str(), entry.mac.c_str());
  }
}

// Function to display node history on the M5StickC Plus LCD
void displayNodeHistory() {
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
  // Dummy data
  updateRoutingTable(1, "192.168.1.2", "AA:BB:CC:DD:EE:01");
  updateRoutingTable(2, "192.168.1.3", "AA:BB:CC:DD:EE:02");
  updateNodeHistory(1);
  updateNodeHistory(2);
}

// Example usage of sending a node packet.
// This demonstrates how to create and send a packet with example data.
void loop() {
  static unsigned long lastUpdateTime = 0;
  const long updateInterval = 10000; // Update interval set to 10 seconds

  // Current time in milliseconds
  unsigned long currentMillis = millis();

  // Check if it's time to update the routing table
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;

    // Add a new dummy node to the routingTable
    int newNodeID = routingTable.size() + 1;
    String newIP = "192.168.1." + String(newNodeID + 1);
    String newMAC = "AA:BB:CC:DD:EE:" + String(newNodeID, HEX);
    updateRoutingTable(newNodeID, newIP, newMAC);

    // Update and display routing table on LCD and serial monitor
    displayRoutingTable();
    displayRoutingTableSerial();
  }

  // Existing logic for node packet transmission
  NodePacket examplePacket = {1, 2, 3, 0.9};
  sendNodePacket(examplePacket);
  
  // Use a delay for demonstration; consider a non-blocking approach for real applications
  delay(10000);
}

