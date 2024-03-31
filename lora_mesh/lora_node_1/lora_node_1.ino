// LoRa 9x_RX/TX
// -*- mode:C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_TX

#include "lora_node.h"

void setup()
{
  Serial.begin(9600);
  delay(100);

  while (!rf95.init())
  {
    Serial.println("SYS: LoRa radio init failed");

    delay(2000);
    while (1)
      ;
  }
  Serial.println("SYS: LoRa radio init OK!");

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ))
  {
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

  setup_routing_table();
  delay(2000);
}

void loop()
{
  /* ========================================================== */
  /* === HANDLING RESPONSE TO ADD NODE TO ROUTING TABLE REQ === */
  /* === & REQUEST FOR FORWARDING CAPACITY BINS TO SERVER   === */
  /* ========================================================== */
  if (rf95.waitAvailableTimeout(1000))
  {
    Packet packet;

    uint8_t len = sizeof(packet);
    if (rf95.recv((uint8_t *)&packet, &len)) 
    {
      if (packet.authKey == AUTH_KEY)
      {
        if (packet.msgType == MSG_TYPE_REQ_FORWARD_NODE)
        {
          handle_node_packet();
        }
        else if (packet.msgType == MSG_TYPE_CAPACITY)
        {
          handle_capacity_packet(packet.data.capacityPacket);
        }
      }
    }
  }
  /* ========================================================== */
  /* === HANDLING RESPONSE TO ADD NODE TO ROUTING TABLE REQ === */
  /* === & REQUEST FOR FORWARDING CAPACITY BINS TO SERVER   === */
  /* ========================================================== */

  /* ========================================================== */
  /* === HANDLING TO SEND REQ TO ADD NODE TO ROUTING TABLE ==== */
  /* === & WHEN CAPACITY OF BIN FULL SEND REQUEST          ==== */
  /* ========================================================== */
  if (connectedNodes == 0 || (connectedNodes < 2 && millis() - lastReceivedForwardingNode >= requestForwardingNodeInterval))
  {
    forward_node_packet();
  }
  else
  {
    if (binCapacity >= ALERT_THRESHOLD)
    {
      if (!alertSent)
      {
        forward_capacity_packet(NODE_ID, binCapacity);
      }
    }
    else
    {
      alertSent = false;
    }
  }
  /* ========================================================== */
  /* === HANDLING TO SEND REQ TO ADD NODE TO ROUTING TABLE ==== */
  /* === & WHEN CAPACITY OF BIN FULL SEND REQUEST          ==== */
  /* ========================================================== */
}

/* ========================================================== */
/* ================= ROUTING TABLE FUNCTIONS ================ */
/* ========================================================== */
void add_to_routing_table(uint8_t nodeId)
{
  if (connectedNodes < MAX_NODES)
  {
    Node node;

    node.nodeId = nodeId;
    routingTable[connectedNodes] = node;
    connectedNodes++;

    Serial.println("SYS: Add node to the routing table");
    print_routing_table();
  }
  else
  {
    Serial.print("SYS: Routing table is full");
  }
}

void remove_from_routing_table()
{
  if (connectedNodes > 0)
  {
    for (int i = 0; i < connectedNodes - 1; i++)
    {
      routingTable[i] = routingTable[i + 1];
    }
    connectedNodes--;

    Serial.println("SYS: Removed first node from the routing table");
    print_routing_table();
  }
  else
  {
    Serial.println("SYS: No nodes to remove from the routing table");
  }
}

void setup_routing_table()
{
  // take nodeid 0 as sever node
  add_to_routing_table(0);
}

void print_routing_table()
{
  Serial.println("--------------------------");
  Serial.println("| Routing Table (NodeID) |");

  for (int i = 0; i < connectedNodes; i++)
  {
    Serial.print("|           ");
    Serial.print(routingTable[i].nodeId);
    Serial.println("            |");
  }
  Serial.println("--------------------------");
}
/* ========================================================== */
/* ================= ROUTING TABLE FUNCTIONS ================ */
/* ========================================================== */

/* ========================================================== */
/* ============== CAPACITY HANDLING FUNCTIONS =============== */
/* ========================================================== */
void add_to_capacity_list(CapacityPacket cpacket)
{
  if (capacityPackets < MAX_CAPACITY_PACKETS)
  {
    processCapacityPackets[capacityPackets] = cpacket;
    capacityPackets++;

    // Swap elements by prioritizing bin capacity
    for (int i = 1; i < capacityPackets; i++) 
    {
      if (processCapacityPackets[i - 1].binCapacity < processCapacityPackets[i].binCapacity) 
      {
        CapacityPacket temp = processCapacityPackets[i];
        processCapacityPackets[i] = processCapacityPackets[i - 1];
        processCapacityPackets[i - 1] = temp;
      }
    }

    Serial.println("SYS: Add packet to the capacity list");
  }
  else
  {
    Serial.print("SYS: Capacity list is full");
  }
}

void remove_from_capacity_list()
{
  if (capacityPackets > 0)
  {
    for (int i = 0; i < capacityPackets - 1; i++)
    {
      processCapacityPackets[i] = processCapacityPackets[i + 1];
    }
    capacityPackets--;

    Serial.println("SYS: Removed first capacity packet from list");
  }
  else
  {
    Serial.println("SYS: No capacity packets to remove from the list");
  }
}
/* ========================================================== */
/* ============== CAPACITY HANDLING FUNCTIONS =============== */
/* ========================================================== */

/* ========================================================== */
/* ============ TRANSMITTING/RECEIVING FUNCTIONS ============ */
/* ========================================================== */
NodePacket construct_node_packet()
{
  Node node;
  NodePacket nodePacket;

  node.nodeId = NODE_ID;
  nodePacket.node = node;

  return nodePacket;
}

CapacityPacket construct_capacity_packet(uint8_t alertId, uint8_t binCapacity)
{
  Node alertNode;
  Node senderNode;
  Node receiverNode;
  CapacityPacket capacityPacket;

  alertNode.nodeId = alertId;
  senderNode.nodeId = NODE_ID;
  receiverNode.nodeId = routingTable[0].nodeId;
  capacityPacket.alertNode = alertNode;
  capacityPacket.senderNode = senderNode;
  capacityPacket.receiverNode = receiverNode;
  capacityPacket.binCapacity = binCapacity;

  return capacityPacket;
}

AckPacket construct_ack_packet(uint8_t alertId, uint8_t receiverId, uint8_t msgType)
{
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

void sendPacket(const uint8_t *data, uint8_t len)
{
  if (rf95.send(data, len))
  {
    // Serial.println("Packet forwarded successfully");
    rf95.waitPacketSent();
  }
  else
  {
    Serial.println("Packet forwarding failed");
  }
}

void handle_node_packet()
{
  Serial.println("REQUEST: Received to be a node's forwarding node");
  NodePacket nodePacket = construct_node_packet();
  Packet packet;

  packet.msgType = MSG_TYPE_RES_FORWARD_NODE;
  packet.data.nodePacket = nodePacket;

  uint8_t len = sizeof(packet);
  sendPacket((uint8_t *)&packet, &len);
  Serial.println("RESPONSE: Sent confirmation to be a forwarding node");
}

void handle_capacity_packet(CapacityPacket &cpacket)
{
  // Ensure that the capacityPacket is for the correct forwarding node
  if (cpacket.receiverNode.nodeId == NODE_ID)
  {
    Serial.println("REQUEST: Received to forward capacity packet, forwarding...");

    /* ========================================================== */
    /* === PACKET PRIORITY BY CAPACITY BEFORE FORWARDING == */
    /* ========================================================== */
    add_to_capacity_list(cpacket);
    forward_capacity_packet(processCapacityPackets[0].alertNode.nodeId, processCapacityPackets[0].binCapacity);
  }
}

void forward_node_packet()
{
  Serial.println("REQUEST: Add forwarding node...");
  NodePacket sendRequestingNode = construct_node_packet();
  Packet packet;
  packet.authKey = AUTH_KEY;
  packet.msgType = MSG_TYPE_REQ_FORWARD_NODE;
  packet.data.nodePacket = sendRequestingNode;

  uint8_t len = sizeof(packet);
  sendPacket((uint8_t *)&packet, &len);

  uint8_t ackMessage = MSG_TYPE_ACK_FAILURE;

  unsigned long startTime = millis();
  unsigned long timeout = 15000; // 15 seconds timeout

  // Check within 15 seconds if there is reply to add node to routing table
  while ((millis() - startTime) <= timeout)
  {
    if (rf95.waitAvailableTimeout(2000))
    {
      Packet packet;

      uint8_t len = sizeof(packet);
      if (rf95.recv((uint8_t *)&packet, &len)) {
        if (packet.authKey == AUTH_KEY && packet.msgType == MSG_TYPE_RES_FORWARD_NODE && packet.data.nodePacket.node.nodeId == 0)
        {
          Serial.println("RESPONSE: Node's acknowledgement as forwarding node");
          add_to_routing_table(packet.data.nodePacket.node.nodeId);
          ackMessage = MSG_TYPE_ACK_SUCCEED;
          break;
        }
      }
    }
    else
    {
      Serial.println("ACK: Not received, attempting to retransmit");
      uint8_t len = sizeof(packet);
      sendPacket((uint8_t *)&packet, &len);
    }
  }

  if (ackMessage == MSG_TYPE_ACK_FAILURE)
  {
    Serial.println("ACK: Not received, no nodes accepted as forwarding node");
  }

  lastReceivedForwardingNode = millis();
}

void forward_capacity_packet(uint8_t alertId, uint8_t binCapacity)
{
  Serial.println("REQUEST: Foward capacity packet...");
  Packet packet;
  packet.authKey = AUTH_KEY;
  packet.msgType = MSG_TYPE_CAPACITY;
  packet.data.capacityPacket = construct_capacity_packet(alertId, binCapacity);

  uint8_t len = sizeof(packet);
  sendPacket((uint8_t *)&packet, &len);

  uint8_t ackMessage = MSG_TYPE_ACK_FAILURE;

  // Wait for ACK or timeout
  unsigned long startTime = millis();
  unsigned long timeout = 15000; // 15 seconds timeout

  while ((millis() - startTime) <= timeout)
  {
    if (rf95.waitAvailableTimeout(5000))
    {
      AckPacket ackPacket;

      uint8_t len = sizeof(ackPacket);
      if (rf95.recv((uint8_t *)&ackPacket, &len))
      {
        if (ackPacket.receiverNode.nodeId == NODE_ID && ackPacket.authKey == AUTH_KEY && ackPacket.msgType == MSG_TYPE_ACK_SUCCEED)
        {
          Serial.println("ACK: Received, capacity alert sent to server");
          ackMessage = MSG_TYPE_ACK_SUCCEED;
          if (ackPacket.alertNode.nodeId == NODE_ID)
          {
            alertSent = true;
          }
          else
          {
            Serial.println("RESPONSE: Forwarding ACK to alert node");
            ackPacket = construct_ack_packet(processCapacityPackets[0].alertNode.nodeId, processCapacityPackets[0].senderNode.nodeId, MSG_TYPE_ACK_SUCCEED);
            uint8_t len = sizeof(ackPacket);
            sendPacket((uint8_t *)&ackPacket, &len);
            remove_from_capacity_list();
          }
          break;
        }
      }
    }
    else
    {
      Serial.println("ACK: Not received, attempting to retransmit");
      uint8_t len = sizeof(packet);
      sendPacket((uint8_t *)&packet, &len);
    }
  }

  if (ackMessage == MSG_TYPE_ACK_FAILURE)
  {
    Serial.println("Ack: Not received, forwarding node is down");
    remove_from_routing_table();
  }
}
/* ========================================================== */
/* ============ TRANSMITTING/RECEIVING FUNCTIONS ============ */
/* ========================================================== */