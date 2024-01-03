# Smart Meter Reader

## Overview
This project utilizes the ESP32-C3 microcontroller to interface with a SmartMeter using the MBus protocol. The collected data is then transmitted over NB-IoT (Narrowband Internet of Things) using a custom communication protocol layered on top of UDP (User Datagram Protocol).

## Features
* ESP32-C3 Integration: The project is build ontop of the esp32-c3 as a application processor. It uses its cryptocrafic functions for data encryption and decryption, but could be replaced with a different mcu.
* Minimal MBus Implementation: Its desigend to read the Smart Meter Data using the MBus Transceiver NCN5150. In the current form only reading data is supported since the Kaifa MA309M doenst support writing to it.
* NB-IoT Connectivity: Since its not common to have a wifi connection at the smartmeter, the SIM7020E module is used to transmit the data over NB-IoT.
* Micro Guard UDP: A custom protocol ontop of UDP is used to authenticate at the server and encrypt the data.

## Hardware
The project uses a custom pcb containing the ESP32-C3, NCN5150 and SIM7020E module.

