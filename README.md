# Internet of Bicycles (#Devices)

**University of Muenster**

* Institute for Geoinformatics
* Course: Internet of Bicycles
* Summer term 2015 

## Goal

We are building a prototype anti-theft device for bicycles. We are utilizing the Sigfox radio network to transmit the current lock status of the bike, its position and auxiliary environment data (such as temperature).

When the bike is in use, the device should transmit collected data directly with a smartphone over Bluetooth. This is due to limitations with the Sigfox radio network in regards to number and size of messages, as well as reduced reliability when moving/driving with the transmitter.

## Hardware

Components:
* Snootlab Akeru beta 3.3 (Arduino UNO based device with TD1208 Sigfox modem)
* [RedBearLab.com Bluetooth Low Energy Shield v2.1](http://redbearlab.com/bleshield/)
* [Seeed Studio Grove Base Shield 2.0](http://www.seeedstudio.com/wiki/Grove_-_Base_shield_v2)
* [Seeed Studio Grove GPS 1.1](http://www.seeedstudio.com/wiki/Grove_-_GPS)
* Seeed Studio Grove DHT11 v1.1 Temperature & Humidity Sensor

### Notes about the GPS receiver

Our Seeed Studio Grove GPS shield is the older version that comes with an u-blox 6 chip. Useful resources for usage and configuration can be found here:

* [u-center software](https://www.u-blox.com/de/evaluation-tools-a-software/u-center/u-center.html) (when chip is directly connected to a PC via USB to Serial adapter)
* [u-blox 6 hardware manual](http://www.u-blox.com/images/downloads/Product_Docs/u-blox6_ReceiverDescriptionProtocolSpec_(GPS.G6-SW-10018).pdf)
* UKHAS Wiki pages about configuring the u-blox 6 at runtime with Arduino: [1](https://ukhas.org.uk/guides:ublox6), [2](https://ukhas.org.uk/guides:ublox_psm)

## Software

Required libraries:
* [Akeru library](http://snoot.it/akerulib)
* [TinyGPS library](https://github.com/mikalhart/TinyGPS/releases)
* [NordicSemiconductor BLE SDK library](https://github.com/NordicSemiconductor/ble-sdk-arduino)
* [Seeed Studio DHT library](https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor)

Copy the extracted libraries into a folder in your ```~\Documents\Arduino\libraries```, make sure your folder names don't have any ```-``` or other non alphanumeric characters in them or else the Arduino IDE will complain.

Finally install the ```theftprotection_BLE\theftprotection_BLE.ino``` on your device.