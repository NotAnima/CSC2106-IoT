#include <M5StickCPlus.h>
#undef min
#undef max // It's also a good idea to undefine max for consistency
#include "Custom_WiFi.h" // Make sure Custom_WiFi.h does not redefine receivedCallback if it's already defined in WiFi_Node.ino

void setup() {
  M5.begin();
  Serial.begin(115200);


  mesh.setDebugMsgTypes(ERROR | DEBUG);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &ts, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&onNewConnectionCallback);
  mesh.onDroppedConnection(&onDroppedConnectionCallback);

  // Display NodeID on LCD
  // displayLCD();
  M5.Lcd.println(preferredServer);

  // // Setup tasks
  // ts.init();
  ts.addTask(taskGetBinCapacity);
  ts.addTask(tSendCustomMessage);
  ts.addTask(taskDisplayLCD);
  taskGetBinCapacity.enable();
  tSendCustomMessage.enable();
  taskDisplayLCD.enable();
}

void loop() {
  // ts.execute(); // Execute scheduled tasks
  mesh.update(); // Handle mesh networking
}
