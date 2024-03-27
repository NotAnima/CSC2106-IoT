#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>

/* ========================================================== */
/* ================= RADIOHEAD DEFINITIONS ================== */
/* ========================================================== */
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Frequency must match other nodes in mesh
#define RF95_FREQ 920.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void (*resetFunc)(void) = 0; // declare reset function at address 0
/* ========================================================== */
/* ================= RADIOHEAD DEFINITIONS ================== */
/* ========================================================== */

/* ========================================================== */
/* =============== MESH DEFINITIONS & STRUCTS =============== */
/* ========================================================== */
#define AUTH_KEY 0x01 // Shared secret key to authenticate nodes
#define NODE_ID 2     // Id of this node (randomly generated)
#define MAX_NODES 2
#define MAX_CAPACITY_PACKETS 10
#define ALERT_THRESHOLD 80
#define MSG_TYPE_REQ_FORWARD_NODE 10
#define MSG_TYPE_RES_FORWARD_NODE 14
#define MSG_TYPE_CAPACITY 3
#define MSG_TYPE_ACK_SUCCEED 4
#define MSG_TYPE_ACK_FAILURE 5

struct Node
{
  uint8_t nodeId;
};

struct NodePacket
{
  Node node;
};

struct CapacityPacket
{
  Node alertNode; // the root node that sends alert
  Node senderNode;
  Node receiverNode;
  uint8_t binCapacity;
};

struct AckPacket
{
  Node alertNode; // the root node that sends alert
  Node receiverNode;
  uint8_t authKey;
  uint8_t msgType;
};

union PacketData
{
  NodePacket nodePacket;
  CapacityPacket capacityPacket;
};

struct Packet
{
  uint8_t authKey;
  uint8_t msgType;
  PacketData data;
};

struct Node routingTable[MAX_NODES];

// At anypoint of time the node can enqueue MAX_PROCESS_CAPACITY_PACKETS number of packets
struct CapacityPacket processCapacityPackets[MAX_CAPACITY_PACKETS];
/* ========================================================== */
/* =============== MESH DEFINITIONS & STRUCTS =============== */
/* ========================================================== */

/* ========================================================== */
/* ======================== VARIABLES ======================= */
/* ========================================================== */
uint8_t connectedNodes = 0;  // current number of connected nodes
uint8_t capacityPackets = 0; // current number of capacity packets
uint8_t binCapacity = 80;    // simulated bin capacity (%)
unsigned long lastReceivedForwardingNode = millis();
unsigned long requestForwardingNodeInterval = 10UL * 60 * 1000;
bool alertSent = false;
/* ========================================================== */
/* ======================== VARIABLES ======================= */
/* ========================================================== */

/* ========================================================== */
/* ================ ROUTING TABLE DECLARATION =============== */
/* ========================================================== */
void add_to_routing_table(uint8_t nodeId);
void remove_from_routing_table();
void setup_routing_table();
void print_routing_table();
/* ========================================================== */
/* ================ ROUTING TABLE DECLARATION =============== */
/* ========================================================== */

/* ========================================================== */
/* ============ CAPACITY HANDLING DECLARATION =============== */
/* ========================================================== */
void add_to_capacity_list(CapacityPacket cpacket);
void remove_from_capacity_list();
/* ========================================================== */
/* ============ CAPACITY HANDLING DECLARATION =============== */
/* ========================================================== */

/* ========================================================== */
/* ==== TRANSMITTING/RECEIVING FUNCTIONS DECLARATION ======== */
/* ========================================================== */
NodePacket construct_node_packet(uint8_t msgType);
CapacityPacket construct_capacity_packet(uint8_t alertId, uint8_t binCapacity);
AckPacket construct_ack_packet(uint8_t msgType);

void sendPacket(const uint8_t *data, uint8_t len);

void handle_node_packet();
void handle_capacity_packet(CapacityPacket &packet);

void forward_node_packet();
void forward_capacity_packet(uint8_t alertId, uint8_t binCapacity);
/* ========================================================== */
/* ==== TRANSMITTING/RECEIVING FUNCTIONS DECLARATION ======== */
/* ========================================================== */