# 🕹️ IT FEST ARCADE
### ESP32 · 4×20 I2C LCD · Joystick · Firebase

> A fully hardware-based arcade game station built for **IT Fest 2025** by the CSIT Department.  
> 3 games. No PC needed during play. Scores sync live to a web dashboard via Firebase.

---

## 📸 What It Looks Like

```
┌────────────────────┐
│* IT FEST ARCADE *  │  ← Menu screen on the 4×20 LCD
│ 1. Dino  Run      │
│>2. Snake         <│  ← Currently selected
│ 3. Maze Runner    │
└────────────────────┘
```

---

## 🎮 Games

| # | Game | Controls | Description |
|---|------|----------|-------------|
| 1 | 🦕 **Dino Run** | Joystick UP = jump | Classic endless runner. Avoid the cacti. Speed increases over time. |
| 2 | 🐍 **Snake** | Joystick = steer | Eat food to grow. Don't hit the walls or yourself. |
| 3 | 🧩 **Maze Runner** | Joystick = move | Navigate to the EXIT before the 60-second timer runs out. Collect coins for bonus points. |

### Menu Navigation
- **Joystick UP / DOWN** → scroll through games
- **Hold joystick RIGHT for 1 second** → select / confirm
- No button press needed anywhere — joystick only!

---

## 🔧 Hardware Components

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 Dev Module |
| Display | 4×20 I2C LCD (address `0x27` or `0x3F`) |
| Input | KY-023 Joystick Module |
| Power | 2× 18650 Li-ion batteries + 5V buck converter |
| Prototyping | 830 tie-point breadboard |

---

## 🔌 Wiring

| Joystick Pin | ESP32 GPIO |
|---|---|
| VCC | 3.3V |
| GND | GND |
| VRX | GPIO 32 |
| VRY | GPIO 34 |

| LCD Pin | ESP32 GPIO |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | GPIO 26 |
| SCL | GPIO 27 |

> ⚠️ **Important:** Joystick VCC must go to **3.3V**, not 5V. ESP32 ADC pins are 3.3V max.  
> GPIO 34 and 35 are **input-only** on ESP32 — do not use them for output.

---

## 📁 Project Structure

```
itfest-arcade/
│
├── itfest_arcade_firebase.ino   # Main ESP32 sketch (3 games + Firebase)
├── itfest_dashboard.html        # Web dashboard (player entry + live leaderboard)
└── README.md                    # This file
```

---

## ⚙️ Setup Guide

### 1. Arduino IDE Setup

Make sure you have these installed:

- **Arduino IDE 2.x** → [arduino.cc/en/software](https://arduino.cc/en/software)
- **ESP32 Board Package** → Arduino IDE → File → Preferences → add this URL to Additional Board Manager URLs:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
  Then: Tools → Board → Boards Manager → search `esp32` → install by Espressif Systems

### 2. Install Required Libraries

Go to **Sketch → Include Library → Manage Libraries** and install:

| Library | Author |
|---------|--------|
| `LiquidCrystal I2C` | Frank de Brabander |
| `Firebase ESP32 Client` | Mobizt |

### 3. Configure WiFi and Firebase

Open `itfest_arcade_firebase.ino` and fill in your credentials at the top:

```cpp
#define WIFI_SSID  "your_wifi_name"
#define WIFI_PASS  "your_wifi_password"
```

Firebase is already configured for this project. Do not change `FB_HOST` or `FB_AUTH`.

> ⚠️ **ESP32 only supports 2.4GHz WiFi.** If your router has both 2.4GHz and 5GHz bands, make sure you connect to the 2.4GHz one.

### 4. Flash the ESP32

1. Connect ESP32 to your PC via USB
2. Tools → Board → `ESP32 Dev Module`
3. Tools → Port → select the correct COM port
4. Click **Upload**

### 5. Open the Dashboard

Just open `itfest_dashboard.html` in any browser — no server needed. It connects directly to Firebase and updates in real time.

---

## 🌐 How Firebase Works

```
Player types name       ESP32 reads name        Game runs on LCD
on dashboard      →     from Firebase      →    (no PC needed)
                              ↓
                    Score posted to Firebase
                              ↓
                    Dashboard leaderboard
                       updates live ⚡
```

### Firebase Database Structure

```
dino-game-59371/
├── currentPlayer/
│   ├── name        ← dashboard writes here, ESP32 reads
│   ├── ready       ← true when player registered
│   └── timestamp
│
└── scores/
    ├── dino/
    │   └── PlayerName/
    │       ├── name
    │       ├── score
    │       └── game
    ├── snake/
    │   └── PlayerName/  ...
    └── maze/
        └── PlayerName/  ...
```

---

## 🐛 Troubleshooting

| Problem | Likely Cause | Fix |
|---------|-------------|-----|
| LCD stays blank | Wrong I2C address | Change `0x27` to `0x3F` in the `.ino` |
| LCD shows garbage | Wrong wiring | Check SDA→26, SCL→27 |
| Joystick not responding | VCC on 5V | Move joystick VCC to 3.3V |
| SW button not working | GPIO 35 has no pull-up | Use GPIO 26 or 27 for SW instead |
| WiFi won't connect | 5GHz band | Use 2.4GHz network only |
| WiFi won't connect | Wrong password | Passwords are case-sensitive |
| Firebase not posting | WiFi not connected | Check serial monitor at 115200 baud |
| `ctags` error in Arduino IDE | Temp file stuck | Restart Arduino IDE, or delete `%temp%` folder |
| `FirebaseESP32.h` not found | Library not installed | Install `Firebase ESP32 Client` by Mobizt |

---

## 🏫 About This Project

This project was built to show junior students what hardware programming looks like in practice — not just blinking an LED, but a real interactive system with:

- **Embedded C++** running on ESP32
- **I2C communication** between ESP32 and LCD
- **Analog input** reading from a joystick module
- **WiFi networking** connecting to the cloud
- **Firebase Realtime Database** for live data sync
- **Web frontend** (HTML/CSS/JS) consuming that live data

Every component was chosen deliberately. The joystick uses analog GPIO pins. The LCD uses I2C to save pins. The ESP32 handles WiFi natively. The batteries make it portable. This is what a real embedded project looks like.

---

## 👨‍💻 Built By

** K3NT0Z4K1 - ICS - IT Fest 2026**

---

## 📄 License

For educational use within the institute for computer studies.