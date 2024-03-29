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
const char* ssid = "SSID";
const char* password = "PASS";

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

#endif