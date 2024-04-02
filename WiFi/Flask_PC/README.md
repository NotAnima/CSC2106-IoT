# Guide to Setup

### Running the Flask Application
To run the Flask application, execute the following command:<br>
`flask --app main run`<br>
You can access the application at http://127.0.0.1:5000/.

### Setting Up MQTT Broker
#### One. Set up MQTT Broker on your laptop
  Update the `mosquitto.conf` file with the following configuration:
  ```
  listener 1883
  listener 9001
  protocol websockets
  ```

  Start the MQTT broker with the updated configuration:<br>
  `cd "C:\Program Files\mosquitto"` <!-- Depends on where your mosquitto folder is --><br>
  `mosquitto.exe -v -c mosquitto.conf`

========================================================================================

#### Two. Subscriber (Flask)
  In `Flask_PC/templates/index.html`<br>
  line 153, Replace host with the broker’s IP (Your PC IP Address).<br>
  line 158, Replace subscribedTopic with your published topic.<br>
  line 154, Port to remain as "9001"

========================================================================================

#### Threee. Setting Up MQTT Client (on the M5StickC Plus)
  Configure the MQTT client on M5StickC Plus using API libraries and the provided sample code `mqtt.ino`.<br>
  Ensure correct SSID, password, and broker IP. Test communication with the PC subscriber.<br>
  E.g. broker’s IP is Your PC IP Address, Port is "1883"

========================================================================================
