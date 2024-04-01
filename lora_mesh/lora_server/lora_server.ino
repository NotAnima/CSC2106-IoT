// LoRa 9x_RX/TX
// -*- mode:C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_TX

#include "lora_server.h"

void setup() {
  Serial.begin(9600);
  delay(100);

  while (!rf95.init()) {
    Serial.println("SYS: LoRa radio init failed");

    delay(2000);
    while (1)
      ;
  }
  Serial.println("SYS: LoRa radio init OK!");

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("SYS: setFrequency failed");
    while (1)
      ;
  }
  Serial.print("SYS: Set Freq to ");
  Serial.println(RF95_FREQ);

  // Defaults after init are 915.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(13, false);

  delay(2000);
}

void loop() {
  /* ========================================================== */
  /* === HANDLING RESPONSE TO ADD NODE TO ROUTING TABLE REQ === */
  /* === & REQUEST FOR HANDLING CAPACITY BINS AT SERVER     === */
  /* ========================================================== */
  if (rf95.waitAvailableTimeout(1500)) {
    Packet packet;

    uint8_t len = sizeof(packet);
    if (rf95.recv((uint8_t *)&packet, &len)) 
    {
      if (packet.authKey == AUTH_KEY) {
        if (packet.msgType == MSG_TYPE_REQ_FORWARD_NODE) {
          handle_node_packet(packet.data.nodePacket);
        } else if (packet.msgType == MSG_TYPE_CAPACITY) {
          handle_capacity_packet(packet.data.capacityPacket);
        }
      }
    }
  }
  /* ========================================================== */
  /* === HANDLING RESPONSE TO ADD NODE TO ROUTING TABLE REQ === */
  /* === & REQUEST FOR HANDLING CAPACITY BINS AT SERVER     === */
  /* ========================================================== */
}
/* ========================================================== */
/* ============ TRANSMITTING/RECEIVING FUNCTIONS ============ */
/* ========================================================== */
NodePacket construct_node_packet() {
  Node node;
  NodePacket nodePacket;

  node.nodeId = NODE_ID;
  nodePacket.node = node;

  return nodePacket;
}

AckPacket construct_ack_packet(uint8_t alertId, uint8_t receiverId, uint8_t msgType) {
  Node alertNode;
  Node receiverNode;
  AckPacket ackPacket;

  alertNode.nodeId = alertId;
  receiverNode.nodeId = receiverId;
  ackPacket.alertNode = alertNode;
  ackPacket.receiverNode = receiverNode;
  ackPacket.authKey = AUTH_KEY;
  ackPacket.msgType = msgType;

  return ackPacket;
}

void sendPacket(const uint8_t *data, uint8_t len) {
  if (rf95.send(data, len)) {
    // Serial.println("Packet forwarded successfully");
    rf95.waitPacketSent();
  } else {
    Serial.println("Packet forwarding failed");
  }
}

void handle_node_packet(NodePacket &packet) {
  if (packet.node.nodeId != 2 || packet.node.nodeId != 3) {
    Serial.println("REQUEST: Received to be a node's forwarding node");
    NodePacket nodePacket = construct_node_packet();
    Packet packet;
    packet.msgType = MSG_TYPE_RES_FORWARD_NODE;
    packet.data.nodePacket = nodePacket;

    uint8_t len = sizeof(packet);
    sendPacket((uint8_t *)&packet, &len);

    Serial.println("RESPONSE: Sent confirmation to be a forwarding node");
  }
}

void handle_capacity_packet(CapacityPacket &cpacket) {
  // Ensure that the capacityPacket is for the correct forwarding node
  if (cpacket.receiverNode.nodeId == NODE_ID) {
    Serial.println("RESPONSE: Received capacity packet at server");

    AckPacket ackPacket = construct_ack_packet(cpacket.alertNode.nodeId, cpacket.senderNode.nodeId, MSG_TYPE_ACK_SUCCEED);
    
    uint8_t len = sizeof(ackPacket);
    sendPacket((uint8_t *)&ackPacket, &len);

    Serial.print("RESPONSE: Bin capacity for node ");
    Serial.print(cpacket.alertNode.nodeId);
    Serial.print(" is at ");
    Serial.print(cpacket.binCapacity);
    Serial.println("%");
  }
}
/* ========================================================== */
/* ============ TRANSMITTING/RECEIVING FUNCTIONS ============ */
/* ========================================================== */