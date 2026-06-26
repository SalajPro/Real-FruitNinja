# 🍉 ESP32 Motion Fruit Ninja

A real-time **Fruit Ninja** clone controlled entirely by **physical hand movements** using an **ESP32** and **MPU6050** motion sensor.

Instead of swiping on a touchscreen, players swing the ESP32 in the air to slice fruits. The ESP32 hosts the game over Wi-Fi, allowing anyone connected to the same network to play directly from a web browser—no installation required.

---

## ✨ Features

- 🎮 Motion-controlled gameplay
- 📶 ESP32 hosts its own web server
- 🌐 Play from any browser on your phone, tablet, or PC
- ⚡ Real-time sensor communication
- 🧭 Interactive calibration before every game
- 📈 Live acceleration visualization during calibration
- 🎯 User-specific motion thresholds
- 🍉 Classic Fruit Ninja gameplay
- 💥 Responsive slicing effects and scoring
- 📱 Mobile-friendly interface

---

## 🛠 Hardware

- ESP32
- MPU6050 Accelerometer/Gyroscope
- USB Cable
- Wi-Fi Network

---

## 📦 Software

- Arduino IDE
- ESP32 Board Package
- Required libraries:
  - WiFi
  - WebServer
  - Wire

---

## 🚀 How It Works

1. Flash the Arduino sketch to the ESP32.
2. Connect the ESP32 to your Wi-Fi.
3. Open the Serial Monitor.
4. Copy the IP address displayed.
5. Open the address in any web browser.
6. Press **Start Calibration**.
7. Complete the guided calibration:
   - Hold still
   - Move right
   - Move left
   - Move up
   - Move down
8. Calibration automatically generates motion thresholds.
9. Start slicing fruits!

---

## 📂 Project Structure

```
.
├── main.ino          # ESP32 firmware
└── README.md
```

---

## 🎯 Calibration

The game performs a live calibration every time before starting.

Instead of using fixed motion values, it learns each player's movement range, making gameplay:

- More accurate
- More responsive
- Personalized
- Less prone to accidental slices

---

## 🌐 Web Interface

The ESP32 serves the complete game directly from its internal memory.

No external server is required.

Simply connect to:

```
http://ESP32_IP_ADDRESS
```

If you like this project, consider giving it a ⭐ on GitHub!
