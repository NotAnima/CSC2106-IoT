
#include <M5StickCPlus.h>
#include "Server_Custom_WiFi.h"

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
  receivePing();
  // receivePacket();
  // timedSendPacket();

  if (currentMillis - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = currentMillis; 
    displayInfo();
    }
  

  // Check and mark inactive nodes from routing table every 10 seconds
  if (currentMillis - lastRoutingTableCheck >= 10000) { // 10 seconds
    lastRoutingTableCheck = currentMillis;
    markInactiveNodesAsOff();
  }

  delay(1000);
}
