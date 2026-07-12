# 🚂 ESP32 Railway Gate Control System

An intelligent railway level crossing automation system built using **ESP32**, **SW-420 vibration sensors**, **IR sensors**, **servo motors**, and **buzzers**. The project automatically detects approaching trains, controls railway gates, provides warning alerts, and incorporates multiple safety mechanisms to improve crossing reliability.

> **Key Idea:** Instead of relying only on IR sensors, this project introduces a **vibration sensor authentication layer** that validates train movement before critical actions such as gate closure or reset are performed. This helps reduce false triggers caused by pedestrians, animals, or environmental disturbances.

---

## 📖 Project Overview

Railway level crossings remain vulnerable to accidents caused by delayed gate operation, poor visibility, and human error. This project demonstrates how an embedded system can automate gate operation using multiple sensors and intelligent software logic.

The ESP32 continuously monitors vibration and infrared sensors to determine train movement, control gate operation, activate warning buzzers, and monitor the crossing for trapped vehicles or pedestrians.

Unlike many student railway gate projects, this system also demonstrates:

- Bidirectional train detection
- Dynamic IR sensor allocation based on train direction
- Vibration-based authentication before SET and RESET operations
- Obstacle detection between the gates
- Modular design for future expansion to dual-track operation
---

## ✨ Key Features

- 🚦 **Automatic Railway Gate Control**  
  Opens and closes the gates automatically based on train movement.

- 📳 **Vibration Sensor Authentication**  
  SW-420 vibration sensors validate train movement before IR sensors are allowed to trigger critical operations, reducing false activations.

- ↔️ **Bidirectional Train Detection**  
  Supports trains approaching from both clockwise and anticlockwise directions using the same sensor arrangement.

- 🔄 **Dynamic IR Sensor Allocation**  
  Selected IR sensors perform different functions depending on the detected train direction, reducing hardware requirements and GPIO usage.

- 🚨 **Automatic Warning System**  
  Buzzers alert nearby pedestrians and vehicles before gate closure.

- 🛡️ **Obstacle Detection**  
  IR sensors monitor the area between the gates. If an obstacle is detected, the affected gate reopens to allow safe passage.

- ⚙️ **ESP32-Based Control Logic**  
  All sensor inputs and actuator outputs are managed by the ESP32 using modular embedded software.

- 🔧 **Modular Architecture**  
  The system is designed so additional sensors or future enhancements can be integrated with minimal changes.
---

## 🛠️ Hardware Used

| Component | Purpose |
|-----------|---------|
| ESP32 DevKit V1 | Main microcontroller |
| SW-420 Vibration Sensors | Early train detection and authentication |
| Flying Fish IR Sensors | Train detection and obstacle detection |
| SG90 Servo Motors | Gate opening and closing |
| Passive Buzzers | Audible warning alerts |
| Breadboard & Jumper Wires | Prototype connections |
| External 5V Power Supply | Powers the hardware |

---

## 💻 Software Used

- Arduino IDE
- Embedded C/C++
- ESP32 Arduino Core
