//************************************************************
// this is a simple example that uses the painlessMesh library to
// connect to a another network and relay messages from a MQTT broker to the nodes of the mesh network.
// To send a message to a mesh node, you can publish it to "painlessMesh/to/12345678" where 12345678 equals the nodeId.
// To broadcast a message to all nodes in the mesh you can publish it to "painlessMesh/to/broadcast".
// When you publish "getNodes" to "painlessMesh/to/gateway" you receive the mesh topology as JSON
// Every message from the mesh which is send to the gateway node will be published to "painlessMesh/from/12345678" where 12345678 
// is the nodeId from which the packet was send.
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

/*===================================================================*/
/*                     Custom Struct Declaration                     */
/*===================================================================*/
struct CustomMessage {
  String payload;
  int priority;
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

/*===================================================================*/
/*                         Global variables                          */
/*===================================================================*/
painlessMesh  mesh;
WiFiClient wifiClient;
SimpleList<uint32_t> nodes;
IPAddress getlocalIP();
IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(192, 168, 68, 103);
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);
// Priority Queue Declaration
std::priority_queue<CustomMessage, std::vector<CustomMessage>, std::function<bool(const CustomMessage&, const CustomMessage&)>> messageQueue(
  [](const CustomMessage& msg1, const CustomMessage& msg2) {
      return msg1.priority > msg2.priority; // Higher priority messages should come first
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

  displayLCD();
}

void loop() {
  mesh.update();
  mqttClient.loop();
  
  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
    displayLCD();
    if (mqttClient.connect("painlessMeshClient")) {
      Serial.println("Inside connected");
      mqttClient.publish("painlessMesh/from/gateway","Ready!");
      mqttClient.subscribe("painlessMesh/to/#");
      String msg;
      msg += "Hello It's the bridge";
      mqttClient.publish("mqtt_bridge", msg.c_str());

    } 
  }
}

void displayLCD(){
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.println("Mesh Network IP: " + mesh.getAPIP().toString());
  M5.Lcd.println(mesh.getNodeId());
  M5.Lcd.println("My IP Address is: " + getlocalIP().toString());
  M5.Lcd.println("Routing Table:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  for (uint32_t node : nodes) {
    M5.Lcd.print("Node ID: ");
    M5.Lcd.println(node);
  }
}


String serializeMessage(const CustomMessage& message) {
  StaticJsonDocument<200> doc;

  doc["payload"] = message.payload;
  doc["priority"] = message.priority;

  String serializedMessage;
  serializeJson(doc, serializedMessage);
  return serializedMessage;
}

CustomMessage deserializeMessage(const String& jsonString) {
  StaticJsonDocument<200> doc;
  deserializeJson(doc, jsonString);

  CustomMessage message;

  message.payload = doc["payload"].as<String>();
  message.priority = doc["priority"];

  return message;
}

void processMessagesFromQueue() {
  while(!messageQueue.empty()) {
    CustomMessage message = messageQueue.top();
    messageQueue.pop();
    // publish to topic [HERE]
    Serial.printf("[Priority Queue] Received message with priority %d: %s\n", message.priority, message.payload.c_str());
  }
}

void addToMessageQueue(const CustomMessage& message) {
  messageQueue.push(message);
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  CustomMessage receivedMessage;
  String topic = "mqtt_bridge";
  receivedMessage = deserializeMessage(msg);
  addToMessageQueue(receivedMessage);
  // add the message to a queue


  // mqttClient.publish(topic.c_str(), msg.c_str());
  // Serial.println("Added message from %u msg=")
}

void onChangedCallback(){
  nodes = mesh.getNodeList();
  displayLCD();
}

// used for when broker sends a message to this bridge network to get the callback targetStr information
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
      mqttClient.publish("painlessMesh/from/gateway", "Client not connected!");
    }
  }
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
