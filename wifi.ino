#include <M5StickCPlus.h>
#include <WiFi.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const int serverPort = 80;
const char* serverAddress = "192.168.1.100";

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
const String neighbors[] = {"NodeA", "NodeB", "NodeC"};

// Queue to store packets with priority
PriorityQueue packetQueue;

// WiFi client instance
WiFiClient client;

// Function prototypes
void connectToWiFi();
void handlePacketTransmission();
void handleRetransmission();
bool transmitPacket(const String& data);
bool receiveACK();

void setup() {
  M5.begin();
  Serial.begin(115200);

  connectToWiFi();
}

void loop() {
  // Check for available packets and enqueue them with priority
  // This is where you would implement your packet generation logic
  handlePacketTransmission();

  handleRetransmission();

  delay(1000);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
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

    // Transmit packet
    Serial.println("Transmitting packet: " + packet.data);
    bool ackReceived = transmitPacket(packet.data);

    // if (!ackReceived) {
    //   // Retransmit packet if ACK not received
    //   for (int i = 0; i < MAX_RETRANSMISSION_ATTEMPTS; i++) {
    //     Serial.println("Retransmitting packet: " + packet.data);
    //     delay(1000); // Adjust delay as needed
    //     ackReceived = transmitPacket(packet.data);
    //     if (ackReceived) {
    //       break;
    //     } else {
    //       // changeNeighbour
    //     }
    //   }
    // }

    // Remove packet from queue if maximum attempts reached or ACK received
    if (ackReceived) {
      packetQueue.dequeue();
    }
  }
}

bool transmitPacket(const String& data) {
  if (!client.connect(serverAddress, serverPort)) {
    Serial.println("Connection to server failed");
    return false;
  }

  client.println(data);
  client.flush();

  return receiveACK();
}

bool receiveACK() {
  if (!client.connected()) {
    Serial.println("Client disconnected from server");
    return false;
  }

  if (client.available()) {
    String response = client.readStringUntil('\n');
    Serial.println("Received ACK: " + response);
    if (response.equals("ACK")) {
      return true;
    }
  }

  return false;
}
