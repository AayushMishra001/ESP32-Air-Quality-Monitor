# IoT-Based Air Quality Monitoring System using ESP32

## Overview

This project implements a low-cost IoT-based air quality monitoring system using an ESP32 microcontroller. The system continuously measures particulate matter concentration along with environmental parameters and computes the Air Quality Index (AQI) with environmental compensation for real-time monitoring.

## Features

* PM1.0, PM2.5 and PM10 measurement
* Temperature, humidity and pressure monitoring
* AQI calculation based on PM2.5 concentration
* Environmental compensation of AQI
* OLED display for local monitoring
* Wi-Fi based web dashboard with JSON API
* Real-time sensor data visualization

## Hardware Used

* ESP32 Development Board
* PM Sensor (UART Interface)
* BME280 Environmental Sensor
* SSD1306 OLED Display

## Communication Protocols

* UART – PM Sensor
* I²C – BME280 & OLED Display
* Wi-Fi – Data transmission
* HTTP & JSON – Web dashboard communication

## Software

* Arduino IDE
* Embedded C++
* ESP32 Wi-Fi Library
* Adafruit BME280 Library
* Adafruit SSD1306 Library

## Repository Contents

* ESP32 source code
* Block diagram
* Hardware prototype
* Dashboard screenshots

## Applications

* Smart Cities
* Environmental Monitoring
* Indoor Air Quality Assessment
* IoT-based Sensor Networks

## Future Improvements

* LoRa-enabled long-range communication
* Cloud database integration
* Mobile application support
* Historical data logging
