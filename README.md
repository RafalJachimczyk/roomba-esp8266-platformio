# Roomba Controller for ESP8266

This codebase is a work in progress for Roomba 612 vacum controlled by Wemos D1 Mini (ESP8266) with OLED Display for debugging and MQTT as a messaging protocol. My "production" version of project does not come with OLED, but the code is left here for reference. 

I plan to add schematics for both versions here.

It is best built using Platform.io - https://platformio.org/ - I highly recommend using this open source build environment for your Arduino/ESP8266 (etc) based IOT projects.

It is my approach to Home Assistant controlled Roomba. It is a work in progress and this documentation does not fully explain how to deploy your own project. 

## Inspiration

The below are the best community based resources about Roomba/ESP8266 builds I could find on the Internet: 
- The Hook Up - https://www.youtube.com/watch?v=t2NgA8qYcFI
- johnboiles - https://github.com/johnboiles/esp-roomba-mqtt
- iRobot - https://www.irobotweb.com/~/media/MainSite/PDFs/About/STEM/Create/iRobot_Roomba_600_Open_Interface_Spec.pdf

This code is a blend of Hookup's and John Boiles approaches. 

## Setup

Copy `src/config_template.h` to `src/config.h` and modify it so that it contains correct values for your environment. 

Modify `platformio.ini` with correct values for your environment.

Build and upload using Platform.io


## TODO

- Monitor ESP 8266 current consumption

- Send ESP 8266 to sleep if battery near dangerous depletion level

- Add schematics for
    - Prototype
    - Production version
