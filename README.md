# Internet of Bicycles (#Devices)

**University of Muenster**

* Institute for Geoinformatics
* Course: Internet of Bicycles
* Summer term 2015 

## Goal

We are building a prototype anti-theft device for bicycles. We are utilizing the Sigfox radio network to transmit the current lock status of the bike, its position and auxiliary environment data (such as temperature).

When the bike is in use, the device should transmit collected data directly with a smartphone over Bluetooth. This is due to limitations with the Sigfox radio network in regards to number and size of messages, as well as reduced reliability when moving/driving with the transmitter.

## Setup

Hardware used:
* Snootlab Akeru beta 3.3 (Arduino UNO based device with TD1208 Sigfox modem)
* [RedBearLab.com Bluetooth Low Energy Shield v2.1](http://redbearlab.com/bleshield/)
* [Seeed Studio Grove Base Shield 2.0](http://www.seeedstudio.com/wiki/Grove_-_Base_shield_v2)
* [Seeed Studio Grove GPS 1.1](http://www.seeedstudio.com/wiki/Grove_-_GPS)
* Seeed Studio Grove DHT11 v1.1 Temperature & Humidity Sensor

Required libraries:
* [Akeru library](http://snoot.it/akerulib)
* [TinyGPS library](https://github.com/mikalhart/TinyGPS/releases)
* [NordicSemiconductor BLE SDK library](https://github.com/NordicSemiconductor/ble-sdk-arduino)
* [Seeed Studio DHT library](https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor)

Copy the extracted libraries into your ```~\Documents\Arduino\libraries```, make sure your folder names don't have any ```-``` or other non alphanumeric characters in them.