# 🏄‍♂️ SurfBeacon

**The intelligent, self-hosted IoT lamp that turns complex surf forecasts into beautiful light.**

<img width="500" alt="surfbeacon" src="https://github.com/user-attachments/assets/3943ea6c-a1bc-4a71-b5f5-34175783e65e" />


## 📖 Introduction

**SurfBeacon** is a custom-built Internet of Things (IoT) device designed to keep surfers connected to the ocean without the need for constant screen time. It sits on your desk or shelf, autonomously monitoring weather data for your favorite surf spots, and visualizes the conditions using fluid, dynamic lighting effects.

Unlike commercial alternatives that require subscriptions or cloud accounts, SurfBeacon is **completely self-contained**. It runs its own web server, performs its own data analysis, and stores your "Secret Spot" coordinates locally on the chip.

### 🙋‍♂️ The Story Behind the Project

I am not a professional developer or an electrical engineer. I am just a noob surfer who loves 3D printing and DIY electronics.

I wanted a way to know if the waves were good without doom-scrolling through Surfline every 10 minutes. I designed the 3D-printed housing and the hardware setup myself. For the firmware, **I utilized AI tools to assist in generating the C++ code** for the ESP32. I then refined, tested, and tweaked the logic to create a stable, reliable device.

What started as a fun experiment is now a fully functional tool that I use daily.

-----

## 🌟 Key Features

  * **📱 Phone-Free Monitoring:** Instant visual feedback on surf conditions. If it glows, there are waves.
  * **🔒 Privacy-First Architecture:** 100% local. Your spot locations and preferences are stored on the ESP32's flash memory. No data is sent to external cloud servers.
  * **🌍 Multi-Spot Tracking:** Monitor multiple locations. The lamp automatically cycles through spots that meet your criteria.
  * **🧠 Custom Logic Engine:** A simple, but customizable scoring system that accounts for Swell Height, Period, Direction, and Wind analysis.
  * **🎨 Dynamic Lighting:** The LED animation isn't static. It breathes and moves faster as the swell energy increases.
  * **🔔 Telegram Integration:** Get a notification sent to your phone when conditions are good.
  * **🔌 Safe USB-C Power:** Software-managed power limits ensure the device runs safely off a standard computer port or phone charger.

-----

## 🏗️ Hardware Architecture

The project is built around the **ESP32** microcontroller, chosen for its dual-core processor and integrated WiFi/Bluetooth capabilities.

### Components

1.  **Microcontroller:** ESP32 Development Board (WROOM-32).
2.  **Visual Output:** WS2812B Addressable LED Strip.
      * *Configuration:* The code is optimized for a ring or strip of **30 LEDs**, but can be modified.
      * *Pinout:* Data Line connected to **GPIO 16**.
3.  **Power Supply:** Standard USB-C Cable (with some ESP32 boards, USB-C to USB-C modern cables might not work, so it could be easier to use USB-C to USB-A cable.
4.  **Housing:** Custom 3D Printed Enclosure (Base + Diffuser + LEDs support).

### ⚡ Power Management

To keep the device simple, I avoided external power bricks.

  * The firmware utilizes `FastLED.setMaxPowerInVoltsAndMilliamps(5, 850)`.
  * This strictly limits the LEDs to draw a maximum of **850mA**, allowing the device to be theroretically powered by almost any standard USB port without risk of brownouts or overheating.

-----

## 🧠 The Logic Core: How It Predicts the Surf

### 1\. Data Acquisition

Every hour (configurable), the device wakes up its WiFi modem and queries the **Open-Meteo Marine API**. It fetches:

  * Swell Height & Period
  * Swell Direction
  * Wind Wave Height
  * Wind Speed & Direction (10m)

### 2\. The Scoring Algorithms

The device calculates a **"Surf Quality Score" (0-100+)** using one of three methods, selectable per spot:

#### A. Standard Mode (Energy Calculation)

It calculates raw swell energy:
$$Score = (Height^2 \times Period) \times 1.5$$
*Very simple, but not so accurate.*

#### B. Pro Mode

For spots that are sensitive to wind and direction. This mode applies a series of filters and multipliers:

1.  **Swell Window:** The user defines acceptable compass sectors (e.g., N, NE, E). If the swell comes from a blocked direction (e.g., South), the score is zeroed out.
2.  **Wind Masking:** The algorithm calculates the angle difference between the wind and the ideal "Offshore" direction:
      * **Offshore (±45°):** Score $\times 1.3$ (Bonus)
      * **Cross-shore:** Score $\times 0.9$ or $\times 0.6$ (Penalty)
      * **Onshore:** Score $\times 0.2$ (Heavy Penalty)
3.  **Wind Strength:** Even offshore wind is penalized if it exceeds 30km/h.
4.  **Chop Factor:** If local "Wind Waves" are large compared to the swell, the score is reduced to account for messy surface conditions.

#### C. Custom Formula (Math Override)

For the real pros. You can write your own equation directly in the Web UI using the `tinyexpr` math parser.

-----

## 🔮 Future Roadmap: The Real Platform

**A note on accuracy:**
Currently, SurfBeacon relies on free public API data and the formulas described above. While effective, it is a "best-effort" calculation and it is the best I was able to do on a ESP32 alone.

**I am currently developing a dedicated, professional forecast platform.**
The goal is to move the heavy lifting from the ESP32 to a dedicated server infrastructure. This future platform will include:

  * **Bathymetric Modeling:** Accounting for how the ocean floor shapes the wave at specific spots.
  * **Shadowing Analysis:** Calculating island blockers and refraction.
  * **Live Spot Checks:** Integrating real-time buoy data.

For now, SurfBeacon is a standalone, decentralized entry into this world.

-----

## 💻 The Web Interface (UI/UX)

The user interface is a simple website hosted entirely on the ESP32's limited memory. It features a modern, Dark Mode design inspired by professional dashboard tools.

To access it, once you've connected the lamp to your personal WiFi Network, you'll have to visit ESP32 ip address or **surfbeacon.local** on your browser.

### 🌊 Tab 1: WAVES (Dashboard)

  * **Spot Manager:** Add new spots using an interactive Map Picker. Search for any location worldwide, pin it visually on the map, and the system automatically validates if the selected coordinates are in the ocean.
  * **Thresholds:** Set the "Min Score" (to trigger the lamp) and "Epic Score" (to trigger notifications).
  * **Pro Config:** A visual compass interface to select swell windows and wind directions.

### 💡 Tab 2: LAMP (Fallback)

When there are no waves, SurfBeacon can become an ambient light.


### ⚙️ Tab 3: CONFIG (System)

  * **Animations:** Choose from effects like *Coastal* (Plasma), *Tide* (Fading), or *Static Color* and set your own Epic mode configuration.
  * **Night Mode (DND):** Set specific hours (e.g., 23:00 - 07:00) where the lamp automatically turns off to ensure good sleep.
  * **System Logs:** View the last 10 internal events (updates, errors, notifications) for debugging.
  * **Telegram Setup:** Input your Bot Token and Chat ID.
  * **Updates:** Configure how often the device checks for new weather data (30 mins - 24 hours).

-----

## 🎨 LED Animation Engine

The visual feedback is designed to be organic, not robotic. The firmware maps the **Surf Score** directly to the **Animation Speed (BPM)**.

  * **Weak Surf (Score 15):** The light "breathes" slowly (10 BPM).
  * **Good Surf (Score 40):** The light pulses at a steady rhythm (30 BPM).
  * **Epic Surf (Score 80+):** The light moves rapidly (60+ BPM), mimicking the energy of the ocean.

**The Animations:**

1.  **Breathe:** A classic, smooth pulse.
2.  **Tide:** A flowing animation that moves back and forth.
3.  **Coastal:** A complex noise-based "plasma" effect representing water texture.
4.  **Swell:** A gradient that moves across the strip.
5.  **Breaker:** A "shooting star" effect representing a breaking wave.
6.  **Rainbow:** 

-----

## 🛠️ Installation & Setup Guide

### 1\. Prerequisites

You will need the **Arduino IDE** or **VS Code (PlatformIO)**.

**Required Libraries:**
Install these via the Library Manager:

  * `FastLED` (for LED control)
  * `ArduinoJson` (for API parsing and config files)
  * `WiFiManager` (by tzapu - for easy WiFi setup)
  * `UniversalTelegramBot` (for notifications)
  * `NTPClient` (for time syncing)

**⚠️ Important:**
The project uses also the `tinyexpr` C library for math parsing. You have to download it from https://github.com/codeplea/tinyexpr, then place `tinyexpr.h` and `tinyexpr.c` files in your Arduino IDE libraries folder.

### 2\. Flashing the ESP32

1.  Open `SurfBeacon.ino`.
2.  Select your Board (e.g., ESP32 Dev Module).
3.  **Partition Scheme:** Select **"Huge APP"**. *This is critical to ensure there is space to save your configuration file.*
4.  Upload speed: try with 115200.
5.  Upload the code.

### 3\. First-Time Setup

1.  Power up the device. The LEDs should light up Blue (Loading).
2.  On your phone/PC, search for a WiFi network named **`SurfBeacon_Setup`**.
3.  Connect to it. A "Captive Portal" should open automatically.
4.  Select your home WiFi network and enter the password.
5.  The device will reboot and connect to your internet.
6.  Open your browser and navigate to `http://surfbeacon.local` (or find the device IP in your router settings).

-----

## ⚖️ Disclaimer

**For Hobbyist Use Only.**
This project relies on the Open-Meteo API, which is excellent but provided "as-is." The forecasting logic is my own extreme approximation. Always check official sources and use your own judgment before heading out into the water. The ocean is unpredictable—respect it\!

-----

**See you in the lineup\! 🤙**
