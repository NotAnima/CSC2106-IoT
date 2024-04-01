
#include <M5StickCPlus.h>
#include "Custom_WiFi.h"

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

  // Initialize UDP
  udp.begin(udpPort);

}

void setup() {
  setupWiFi();
  displayInfo();
}

void loop() {
  unsigned long currentMillis = millis();

  sendPing();

  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = currentMillis;
    displayInfo();
    sendPacket();
    removeInactiveNodes();
    printAndRequeuePackets();
  }

  // // Check and remove inactive nodes from routing table every 5 seconds
  // if (currentMillis - lastRoutingTableCheck >= displayInterval) { // 5 seconds
  //   lastRoutingTableCheck = currentMillis;
  // }

  receivePing();
  receivePacket();
}
