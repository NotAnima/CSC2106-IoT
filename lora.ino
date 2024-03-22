#include <SPI.h>
#include <LoRa.h>

#define BAND 433E6
#define NODE_ID "NodeA"

// Packet structure
struct Packet {
  int priority;
  String data;
};

// Priority queue to store packets
struct PriorityQueue {
  Packet packets[100];
  int size = 0;

  void enqueue(Packet packet) {
    int index = size;
    // Algorithm to find position to insert based on priority
    while (index > 0 && packets[index - 1].priority > packet.priority) {
      packets[index] = packets[index - 1];
      index--;
    }
    packets[index] = packet;
    size++;
  }

  Packet dequeue() {
    Packet packet = packets[0];
    for (int i = 1; i < size; i++) {
      packets[i - 1] = packets[i];
    }
    size--;
    return packet;
  }

  bool isEmpty() {
    return size == 0;
  }

  Packet peek() {
    return packets[0];
  }
};

// Define maximum number of retransmission attempts
const int MAX_RETRANSMISSION_ATTEMPTS = 3;

// Define routing table
const String neighbors[] = {"NodeB", "NodeC", "NodeD"};

// Queue to store packets with priority
PriorityQueue packetQueue;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa initialization failed. Check your connections.");
    while (true);
  }
}

void loop() {
  // Check for available packets and enqueue them with priority
  // This is where you would implement your packet generation logic
  handlePacketTransmission();

  handleRetransmission();

  delay(1000);
}

void handlePacketTransmission() {
  // Simulated packet generation
  Packet packet;
  packet.priority = random(1, 4);
  packet.data = "Sample data";
  packetQueue.enqueue(packet);
}

void handleRetransmission() {
  if (!packetQueue.isEmpty()) {
    Packet packet = packetQueue.peek(); // Get the highest priority packet

    Serial.println("Transmitting packet: " + packet.data);
    bool ackReceived = transmitPacket(packet.data);

    // if (!ackReceived) {
    //   // Retransmit packet if ACK not received
    //   for (int i = 0; i < MAX_RETRANSMISSION_ATTEMPTS; i++) {
    //     Serial.println("Retransmitting packet: " + packet.data);
    //     delay(1000);
    //     ackReceived = transmitPacket(packet.data);
    //     if (ackReceived) {
    //       break;
    //     }
    //   }
    }

    // Remove packet from queue if maximum attempts reached or ACK received
    if (ackReceived) {
      packetQueue.dequeue();
    }
  }
}

bool transmitPacket(const String& data) {
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  unsigned long startTime = millis();
  while (millis() - startTime < 2000) { // Wait for 2 seconds for ACK
    if (LoRa.parsePacket()) {
      String receivedData = LoRa.readString();
      if (receivedData == "ACK") {
        return true; // ACK received
      }
    }
  }

  return false; // ACK not received within 2s timeout
}
