# Guide to Setup

### Running the Flask Application
To run the Flask application, execute the following command:
`flask --app main run`
You can access the application at http://127.0.0.1:5000/.

### Setting Up MQTT Broker
One. Set up MQTT Broker on your laptop
  Follow the instructions at https://github.com/drfuzzi/CSC2106_MQTT - Step 1

  Update the `mosquitto.conf` file with the following configuration:
  `listener 1883`
  `listener 9001`
  `protocol websockets`

  Start the MQTT broker with the updated configuration:
  `cd "C:\Program Files\mosquitto"` <!-- Depends on where your mosquitto folder is -->
  `mosquitto.exe -v -c mosquitto.conf`

========================================================================================

Two. Subscriber (Flask)
  In `Flask_Server/templates/index.html`
  line 153, Replace host with the broker’s IP (Your PC IP Address).
  line 158, Replace subscribedTopic with your published topic.
  line 154, Port to remain as "9001"

========================================================================================

Threee. Setting Up MQTT Client (on the M5StickC Plus)
  Follow the instructions at https://github.com/drfuzzi/CSC2106_MQTT - Step 4
  Configure the MQTT client on M5StickC Plus using API libraries and the provided sample code `mqtt.ino`.
  Ensure correct SSID, password, and broker IP. Test communication with the PC subscriber.
  E.g. broker’s IP is Your PC IP Address, Port is "1883"

========================================================================================
