struct Packet {
  int priority;
  String data;
};

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

PriorityQueue packetQueue;
