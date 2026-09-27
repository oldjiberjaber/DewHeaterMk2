# DewHeater Mk2 - Smart Observatory Thermal Controller

An advanced, dual-channel precision dew prevention and thermal management system built for astronomical telescopes and observatory optics on the ESP32 platform using **FreeRTOS** and **PlatformIO**.

---

## 🌟 Key Features

### 1. Dual-Core FreeRTOS Architecture
* **Core 1 (Deterministic Control)**: High-priority (5Hz/200ms) PID heater loop and 1Hz multi-point DS18B20 1-Wire temperature acquisition protected by FreeRTOS mutexes (`xDataMutex`).
* **Core 0 (Display & Telemetry)**: Async HTTP WebServer, Server-Sent Events (SSE), MQTT telemetry publishing, sTune auto-tuner monitoring, and ST7735/SH1107 display rendering.

### 2. Dual-Channel Precision PID Control
* Independent control channels for **CH0 (Guide Scope)** and **CH1 (Main Imaging Scope)**.
* Setpoint dynamically calculated as ambient Dew Point + configurable offset ($\Delta T$).
* Real-time thermal protection and failsafe zero-output shutoff.

### 3. Thermal Soft-Landing Predictive Governor
* Continuous trajectory evaluation: Monitors cooling rates ($^\circ\text{C}/\text{min}$) and target distance.
* Injects predictive braking and conductance feedforward before setpoint overshoot occurs to prevent deep optics undershoot.
* Fully persistent configuration stored in non-volatile flash (NVS).

### 4. Adaptive Self-Learning Thermal Physics Engine
* **Phase 1 (Steady-State Conductance $K_{\text{loss}}$)**: Automatically measures equilibrium holding power per $^\circ\text{C}$ above ambient when settled ($\pm 0.35^\circ\text{C}$). Adapts to changing wind speeds, shroud insulation, and ambient humidity.
* **Phase 2 (Iterative Learning Control $\tau_{\text{lead}}$)**: Optimizes lead dead-time horizons over successive cooling descents for critically damped trajectory arrival.
* Parameters and convergence metrics persist across reboots in NVS (`Preferences` namespace `"dewlearning"`).

### 5. In-System Automated PID Auto-Tuner (`sTune`)
* Open-loop inflection-point step-response parameter identification.
* Generates live in-memory temperature vs PWM response curves on the web dashboard.
* Supports **NoOvershoot** (astronomy recommended), **Ziegler-Nichols**, **Cohen-Coon**, and **Damped Oscillation** rules.
* One-click flash commitment or ephemeral RAM testing.

### 6. Captive Portal & Network Management
* **Fallback Setup AP**: Starts a WiFi SoftAP (`DewHeater-Setup`) with captive portal DNS server at `192.168.4.1` if WiFi is unconfigured or fails to connect.
* **PROGMEM Recovery UI**: Mobile-friendly captive portal page stored directly in flash memory.
* **In-Dashboard WiFi Manager**: Live 2.4GHz network scanner with signal quality readouts, password show/hide, connection status badge, and setup reset.
* Credentials saved securely in NVS (`Preferences` namespace `"wificfg"`).

### 7. Astronomical Solar Calculation & Auto-Dimming
* Built-in solar elevation algorithm computes exact sun position based on latitude/longitude.
* Automatically transitions between day brightness and observatory night vision red dimming.

### 8. Rich Modern Web Dashboard
* Built with responsive CSS Glassmorphism, Chart.js dark-themed telemetry graphs, in-memory 60-minute tracking error logs, and interactive Canvas Gauges.
* Real-time Server-Sent Events (`/events`) updates with zero browser page reloads.

---

## 🛠️ Hardware & Pinout

| Function | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **DS18B20 1-Wire** | `GPIO 4` | Temperature sensor bus (4.7kΩ pullup) |
| **MOSFET CH0** | `GPIO 16` | Guide Scope Heater PWM (5 kHz, 8-bit) |
| **MOSFET CH1** | `GPIO 17` | Main Scope Heater PWM (5 kHz, 8-bit) |
| **MOSFET CH2** | `GPIO 26` | Auxiliary Channel 2 |
| **MOSFET CH3** | `GPIO 27` | Auxiliary Channel 3 |
| **TFT CS** | `GPIO 12` | ST7735 SPI Chip Select |
| **TFT DC** | `GPIO 14` | ST7735 Data / Command |
| **TFT MOSI** | `GPIO 23` | SPI MOSI |
| **TFT SCK** | `GPIO 18` | SPI Clock |
| **TFT LED** | `GPIO 19` | Backlight PWM Dimmer |

---

## 🚀 Getting Started

### 1. Prerequisites
* [Visual Studio Code](https://code.visualstudio.com/) + [PlatformIO IDE Extension](https://platformio.org/)
* Python 3.x with `gzip` support for web asset compression.

### 2. Configuration
Copy the template credentials file:
```bash
cp include/AAA_Credentials.h.example include/AAA_Credentials.h
```
Edit `include/AAA_Credentials.h` with your fallback credentials, MQTT broker IP, and local observatory GPS coordinates.

### 3. Build & Flash
```powershell
# 1. Compress Web Dashboard Assets
python -c "import gzip, shutil; [shutil.copyfileobj(open(f'data/{f}', 'rb'), gzip.open(f'data/{f}.gz', 'wb')) for f in ['index.html', 'script.js', 'style.css']]; print('Gzip complete!')"

# 2. Upload LittleFS Filesystem (Web Assets)
platformio run -t uploadfs

# 3. Build and Flash Firmware (USB Serial or OTA)
platformio run -t upload
```

---

## 📡 MQTT Integration & Topics

| Topic | Type | Payload Example |
| :--- | :--- | :--- |
| `telescope/dew0` | Publish | `{"Temp": 12.4, "Target": 11.2, "PWR": 35}` |
| `telescope/dew1` | Publish | `{"Temp": 13.1, "Target": 11.2, "PWR": 42}` |
| `tele/dewhtr/LWT` | Publish / Retained | `Online` / `Offline` |

---

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
