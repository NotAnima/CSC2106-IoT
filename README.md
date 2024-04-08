# Arduino LoRa and WiFi Mesh Project

## Overview
This project utilizes Arduino boards to create a mesh network using LoRa and WiFi technologies. The mesh network enables communication between multiple nodes over long distances (LoRa) and short distances (WiFi). This README provides an overview of the project, setup instructions, and basic usage guidelines.

## Components
- Cytron LoRa-RFM shield 
- M5StickC PLUS (ESP32-PICO)
- Ultrasonic Sensor (HC-SR04)
- Maker Uno
- Laptop/Raspberry Pi (Flask Server)

## Setup Instructions
- Ensure that [mosquitto](https://mosquitto.org) is installed 
- Alter the config file appropriately to the for mosquitto appropriately to the ports that you want to run the services on
## Required Libraries
Ensure the following libraries are installed in your Arduino IDE:
- [HCSR04 ultrasonic sensor Library](https://github.com/gamegine/HCSR04-ultrasonic-sensor-lib)
- [painlessMesh v1.4.10 and the complementing Libraries](https://gitlab.com/painlessMesh/painlessMesh)
- [xXHash_arduino](https://www.arduino.cc/reference/en/libraries/xxhash_arduino/)
- [M5 Stack Library](https://github.com/m5stack/M5StickC-Plus)
- [Arduino Json Library](https://arduinojson.org)
- [Queue Library](https://www.arduino.cc/reference/en/libraries/queue/)
- [PubSubClient for MQTT](https://www.arduino.cc/reference/en/libraries/pubsubclient/)
