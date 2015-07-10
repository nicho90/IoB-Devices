# Internet of Bicycles (#Devices)

**University of Muenster**

* Institute for Geoinformatics
* Course: Internet of Bicycles
* Summer term 2015 

## Goal

We are building a prototype anti-theft device for bicycles. We are utilizing the Sigfox radio network to transmit the current lock status of the bike, its position and auxiliary environment data (such as temperature).

When the bike is in use, the device should transmit collected data directly with a smartphone over Bluetooth. This is due to limitations with the Sigfox radio network in regards to number and size of messages, as well as reduced reliability when moving/driving with the transmitter.

Hardware used:
* Snootlab Akeru beta 3.3 (Arduino UNO based device with TD1208 Sigfox modem)
* RedBearLab.com Bluetooth Low Energy Shield
* GPS shield