//************************************************************
// This is the server/bridge network as well as the MQTT Publisher to the centralized network
// of whichever WiFi network it is connected to, denoted by the STATION_SSID and STATION_PASSWORD
// those are analogous to wifiSetup() in most other Arduino files

// This bridge server will periodically schedule a task to unload the data packets received from the nodes in
// the mesh network to the MQTT broker specified under the IPAdress of the MQTTBroker, for our example, we would be using our laptops as the broker.
// However, it is worth noting that this can be easily translated into a raspberry pi that runs the MQTT broker
//************************************************************

#include <ArduinoJson.h>
#include <queue>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "M5StickCPlus.h"

#define   MESH_PREFIX     "dustbin"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555

#define   STATION_SSID     "Poh"
#define   STATION_PASSWORD "lsps353ycss"

#define HOSTNAME "MQTT_Bridge"
#define MQTT_TOPIC "dustbinInfo"

/*===================================================================*/
/*                     Custom Struct Declaration                     */
/*===================================================================*/
struct CustomMessage {
  String rootSender;
  float binCapacity;
  uint32_t timestamp;
};

/*===================================================================*/
/*                         Function Prototypes                       */
/*===================================================================*/
void receivedCallback(const uint32_t &from, const String &msg);
void onChangedCallback();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void displayLCD();

// JSON management
String serializeMessage(const CustomMessage& message);
CustomMessage deserializeMessage(const String& jsonString);

// Queue management
void processMessagesFromQueue();
void addToMessageQueue(const CustomMessage& message);

// Tasks
Task taskProcessQueue( TASK_SECOND * 10, TASK_FOREVER, &processMessagesFromQueue);

/*===================================================================*/
/*                         Global variables                          */
/*===================================================================*/
// Task manager
Scheduler taskScheduler;

painlessMesh  mesh;
WiFiClient wifiClient;
SimpleList<uint32_t> nodes;
IPAddress getlocalIP();
IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(192, 168, 68, 103);
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);
// Priority Queue Declaration, makes the earliest receieved messages in the queue to go first
std::priority_queue<CustomMessage, std::vector<CustomMessage>, std::function<bool(const CustomMessage&, const CustomMessage&)>> messageQueue(
  [](const CustomMessage& msg1, const CustomMessage& msg2) {
      return msg1.timestamp < msg2.timestamp; // Earlier messages go first
  }
);

/*===================================================================*/
/*                         Initialize Server                         */
/*===================================================================*/
void setup() {
  Serial.begin(115200);

  M5.begin();

  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
  mesh.onChangedConnections(&onChangedCallback);
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  // Show the initial display
  displayLCD();

  // initialize all scheduled tasks
  taskScheduler.init();
  taskScheduler.addTask(taskProcessQueue);
  taskProcessQueue.enable();
}

void loop() {
  mesh.update();
  mqttClient.loop();
  
  if(myIP != getlocalIP()){

    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
    displayLCD();

    if (mqttClient.connect("painlessMeshClient")) {
      Serial.println("MQTT Client is connected!");
    } 
  }
  
  // executed any queued up tasks in the scheduler
  taskScheduler.execute();

}

void displayLCD(){
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("Mesh Network IP: " + mesh.getAPIP().toString());
  M5.Lcd.println(mesh.getNodeId());
  M5.Lcd.println("My IP Address is: " + getlocalIP().toString());
  M5.Lcd.println("Routing Table:");

  // For showing all the connected nodes in the mesh de-centralized network
  SimpleList<uint32_t>::iterator node = nodes.begin();
  for (uint32_t node : nodes) {
    M5.Lcd.print("Node ID: ");
    M5.Lcd.println(node);
  }
}

// Flattens the message into a jsonDocument that is ready to be sent over the phyiscal layer
String serializeMessage(const CustomMessage& message) {
  StaticJsonDocument<200> doc;

  doc["rootSender"] = message.rootSender;
  doc["binCapacity"] = message.binCapacity;
  doc["timestamp"] = message.timestamp;

  String serializedMessage;
  serializeJson(doc, serializedMessage);
  return serializedMessage;
}

// Inverse of serializeMessage, unflattens the physical data into a JSON and then transforms into a CustomMessage
CustomMessage deserializeMessage(const String& jsonString) {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, jsonString);

  CustomMessage message;

  // Uses the JSON Document with the appropriate keys to create a CustomMessage
  message.rootSender = doc["rootSender"].as<String>();
  message.binCapacity = doc["binCapacity"].as<float>();
  message.timestamp = mesh.getNodeTime(); // assigns the mesh network's sync-ish'd timestamp

  return message;
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("bridge: Received from %u msg=%s, server time: %u\n", from, msg.c_str(), mesh.getNodeTime());
  CustomMessage receivedMessage;
  receivedMessage = deserializeMessage(msg);
  // add the message to a queue to process later so it won't introduce delays and not to drop any packets to provide QOS 1
  addToMessageQueue(receivedMessage);
}

void processMessagesFromQueue() {
  if (mqttClient.connect("painlessMeshClient")) {
    while(!messageQueue.empty()) {
      CustomMessage message = messageQueue.top();
      messageQueue.pop();
      String jsonString = serializeMessage(message);
      Serial.println("JSON String: " + jsonString);
      mqttClient.publish(MQTT_TOPIC, jsonString.c_str());
    }
  } 
}

void addToMessageQueue(const CustomMessage& message) {
  messageQueue.push(message);
}

// whenever the mesh network changes, adding or deleting nodes. It will update the nodes variable and then redisplay the network's routing table
// it will display all the nodes in the network, whether connected directly or indirectly to the bridge serevr
void onChangedCallback(){
  nodes = mesh.getNodeList();
  displayLCD();
}

// used for when broker sends a message to this bridge network to get the callback targetStr information, but not applicable for our usecase thus far
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  char* cleanPayload = (char*)malloc(length+1);
  memcpy(cleanPayload, payload, length);
  cleanPayload[length] = '\0';
  String msg = String(cleanPayload);
  free(cleanPayload);

  String targetStr = String(topic).substring(16);

  if(targetStr == "gateway")
  {
    if(msg == "getNodes")
    {
      auto nodes = mesh.getNodeList(true);
      String str;
      for (auto &&id : nodes)
        str += String(id) + String(" ");
      mqttClient.publish("painlessMesh/from/gateway", str.c_str());
    }
  }
  else if(targetStr == "broadcast") 
  {
    mesh.sendBroadcast(msg);
  }
  else
  {
    uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
    if(mesh.isConnected(target))
    {
      mesh.sendSingle(target, msg);
    }
    else
    {
      mqttClient.publish(MQTT_TOPIC, "Client not connected!");
    }
  }
}

// get THIS bridge node's internal IP address to the bridge's AP. A gateway to centralized internet access
IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
