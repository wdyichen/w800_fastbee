# Fastbee

## Overview

This example demonstrates how a device can connect to the FastBee cloud platform and implement features such as switch control and OTA firmware updates. 

After running the example, the user needs to add products and devices in the cloud platform's device management section, obtain the device's BROKER_URI, USER_NAME, PASSWORD, PRODUCT_ID, and DEVICE_ID information, and configure these in the mqtt.h file. 

Once configured, press and hold the USER button for 3 seconds to enter network configuration mode (the LED will blink twice to indicate successful entry). 

Then, use the "BeeIoT" mini-program or "WMBleWiFi" mini-program for network configuration. 

Upon successful network configuration, the device will connect to the cloud platform and save the configuration information, eliminating the need for repeated configurations. 

At this point, the user can control the switch via the "BeeIoT" mini-program or the cloud platform. When the switch is turned on, the LED will light up, and when the switch is turned off, the LED will turn off. 

Upon successful switch control, the device will receive an MQTT message that also includes the device's temperature information (currently, the temperature is a random value for testing purposes). Additionally, the user can press the USER button briefly to publish an MQTT message for switch control (Optional). 

Furthermore, the user can perform OTA firmware updates through the cloud platform.

## Requirements

1. A W800-Arduino development board.
2. A working AP is required.

## Detailed Tutorial

To be continued ......

