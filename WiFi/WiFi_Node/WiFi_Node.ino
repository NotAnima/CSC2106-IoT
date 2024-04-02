#include <M5StickCPlus.h>
#undef min
#undef max // It's also a good idea to undefine max for consistency
#include "Custom_WiFi.h" // Make sure Custom_WiFi.h does not redefine receivedCallback if it's already defined in WiFi_Node.ino

void setup() {
  M5.begin();
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | DEBUG);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&onNewConnectionCallback);
  mesh.onDroppedConnection(&onDroppedConnectionCallback);

  // Display NodeID on LCD
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  M5.lcd.setRotation(3);
  M5.Lcd.printf("NodeID: %u", mesh.getNodeId());

  // Setup tasks
  ts.init();
  ts.addTask(taskGetBinCapacity);
  ts.addTask(tSendCustomMessage);
  taskGetBinCapacity.enable();
  tSendCustomMessage.enable();
}

void loop() {
  ts.execute(); // Execute scheduled tasks
  mesh.update(); // Handle mesh networking
}
