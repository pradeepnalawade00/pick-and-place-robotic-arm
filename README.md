# 🤖 IoT-Based Pick & Place Robotic Arm

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-Microcontroller-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-Framework-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![IoT](https://img.shields.io/badge/IoT-Enabled-27AE60?style=for-the-badge)
![Servo](https://img.shields.io/badge/Servo-Control-F39C12?style=for-the-badge)

*A wirelessly controlled robotic arm using ESP32, capable of pick-and-place operations via a real-time IoT command interface.*

</div>

---

## 📌 Project Overview

This project is an **IoT-enabled pick-and-place robotic arm** controlled wirelessly through an **ESP32 microcontroller**. The arm features multiple degrees of freedom (DOF) using servo motors and can be commanded in real-time via a web-based interface or mobile application over Wi-Fi.

The system is designed to demonstrate fundamental concepts of **embedded control systems**, **servo actuation**, and **IoT wireless communication** in a robotics context.

---

## ✨ Key Features

- 🌐 **Wireless Control** — Operates over Wi-Fi using ESP32's built-in networking stack
- 🦾 **Multi-DOF Arm** — Controls base rotation, shoulder, elbow, and gripper joints
- 📡 **Real-time Response** — Low-latency servo command delivery (<50ms)
- 🔧 **Modular Design** — Each joint controlled independently for precision placement
- 💡 **Web Interface** — Browser-based control panel accessible from any device on the network

---

## 🏗️ System Architecture

```
┌─────────────────┐         Wi-Fi          ┌──────────────────────┐
│  Control Device │ ─────────────────────► │  ESP32 Controller    │
│  (Phone / PC)   │   HTTP Commands        │  (Web Server + PWM)  │
└─────────────────┘                        └──────────┬───────────┘
                                                      │
                              ┌───────────────────────┼────────────────────┐
                              ▼                       ▼                    ▼
                       ┌────────────┐        ┌─────────────┐     ┌──────────────┐
                       │  Base Servo │        │ Shoulder /  │     │   Gripper    │
                       │  (Rotation) │        │ Elbow Servos│     │   Servo      │
                       └────────────┘        └─────────────┘     └──────────────┘
```

---

## 🛠️ Tech Stack & Components

| Component | Specification | Role |
|-----------|---------------|------|
| **ESP32 Dev Board** | Xtensa LX6 240MHz, Wi-Fi + BT | Main controller + web server |
| **Servo Motors** | SG90 / MG996R | Joint actuation |
| **Power Supply** | 5V 3A regulated | Servo power rail |
| **Arduino Framework** | ESP32 Arduino Core | Firmware development |
| **HTML/JS Web UI** | Hosted on ESP32 | Control interface |

---

## 📁 Repository Structure

```
pick-and-place-robotic-arm/
│
├── firmware/
│   ├── main.ino              # Main ESP32 firmware
│   ├── servo_control.h       # Servo PWM abstraction
│   └── wifi_server.h         # HTTP server and command parsing
│
├── hardware/
│   ├── circuit_diagram.pdf   # Full wiring schematic
│   ├── BOM.xlsx              # Bill of materials
│   └── assembly_guide.pdf    # Physical assembly instructions
│
├── web_ui/
│   └── index.html            # Control panel (served by ESP32)
│
├── docs/
│   └── project_report.pdf    # Full project documentation
│
└── README.md
```

---

## ⚙️ Setup & Deployment

### Prerequisites
- Arduino IDE with **ESP32 board package** installed
- `ESP32Servo` library (`Tools > Manage Libraries > ESP32Servo`)

### Steps
1. **Clone the repository**
   ```bash
   git clone https://github.com/pradeepnalawade00/pick-and-place-robotic-arm.git
   cd pick-and-place-robotic-arm
   ```

2. **Configure Wi-Fi credentials** in `firmware/main.ino`:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

3. **Flash the firmware** to ESP32 via Arduino IDE (Board: `ESP32 Dev Module`, Port: your COM port)

4. **Open Serial Monitor** (115200 baud) — ESP32 will print its IP address

5. **Access control panel** at `http://<ESP32_IP>` in any browser on the same network

---

## 🔌 Wiring Guide

| ESP32 Pin | Connected To |
|-----------|-------------|
| GPIO 13   | Base Servo (Signal) |
| GPIO 12   | Shoulder Servo (Signal) |
| GPIO 14   | Elbow Servo (Signal) |
| GPIO 27   | Gripper Servo (Signal) |
| GND       | Common Ground |
| VIN (5V)  | Servo Power Rail |

---

## 🎯 Learning Outcomes

- Embedded systems design with ESP32 (Wi-Fi SoC)
- PWM servo control for precise angular positioning
- HTTP server implementation on a microcontroller
- IoT system architecture: sensor/actuator + controller + interface

---

## 👤 Author

**Pradeep Nalawade** | ECE Student | Embedded Systems & IoT Enthusiast

[![Portfolio](https://img.shields.io/badge/Portfolio-Visit-A78BFA?style=flat-square)](https://pradeepnalawade00.github.io/)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-0A66C2?style=flat-square&logo=linkedin)](https://www.linkedin.com/in/pradeep-nalawade-950244314/)
[![GitHub](https://img.shields.io/badge/GitHub-Follow-181717?style=flat-square&logo=github)](https://github.com/pradeepnalawade00)
