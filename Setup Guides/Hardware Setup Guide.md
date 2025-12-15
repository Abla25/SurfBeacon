# 🔧 SurfBeacon - Hardware Setup Guide

## Table of Contents
1. [Required Components](#required-components)
2. [Tools & Materials](#tools-materials)
3. [3D Printing the Enclosure](#3d-printing)
4. [LED Strip Preparation](#led-preparation)
5. [ESP32 Wiring](#esp32-wiring)
6. [Physical Assembly](#physical-assembly)
7. [Installing Required Libraries](#installing-libraries)
8. [Flashing the Firmware](#flashing-firmware)
9. [Troubleshooting](#troubleshooting)

---

## 1. Required Components {#required-components}

Alright, let's get the shopping list out of the way. I've added affiliate links below to make your life easier (and yeah, it helps support the project if you use them).

### What You Actually Need

| Component | Specification | Notes |
|-----------|--------------|-------|
| **ESP32 Development Board** | ESP32-WROOM-32 | Any ESP32 dev board works, really |
| **LED Strip** | WS2812B Addressable RGB strip | You'll need 30 LEDs |
| **USB Cable** | USB-C to USB-A | For power and programming (with some esp32 clones, modern USB-C to USB-C cables don't work)|
| **Power Supply** | 5V, 1A minimum | Any phone charger you have lying around |
| **Jumper Wires** | Female-to-Male | 3 | For connecting the LEDs |

**[Affiliate Links Coming Soon]**

### The Optional Stuff

You don't *need* these, but they make life easier:
- **Heat shrink tubing** - Makes your connections look less janky

---

## 2. Tools & Materials {#tools-materials}

### What's in Your Toolbox?

You probably have most of this already:
- Soldering iron (optional, but your future self will thank you for using one)
- Small screwdriver

### For 3D Printing
- Access to a 3D printer (friend's printer, library, makerspace, whatever)
- PLA or PETG filament (white or translucent looks sick for the diffuser)
- Sandpaper if you're picky about finish

---

## 3. 3D Printing the Enclosure {#3d-printing}

### The Three Parts

You'll need to print:
1. **Base** - Where all the electronics hide
2. **LED Support** - Holds the strip in place
3. **Diffuser** - Makes the lights look good instead of blinding you

### Print Settings That Actually Work

Here's what I used (feel free to tweak):
- **Layer Height:** 0.2mm
- **Infill:** 20% (no need to go crazy)
- **Material:** PLA or PETG
- **Supports:** Nope, not needed
- **Print Time:** 4-6 hours total for all three parts

### Material Choices

- **Base:** Whatever color you want. I think coloured is better :)
- **Diffuser:** This matters. Use white, translucent, or clear filament. The light needs to pass through this thing.
- **LED Holder:** Match it with the base or go with white.

### After Printing

Remove any blobs or strings, test fit everything together, and maybe sand the edges if you're feeling fancy.

---

## 4. LED Strip Preparation {#led-preparation}

These LED strips have three wires you care about:
- **+5V** - Power
- **GND** - Ground
- **DIN** - Data signal

### Getting Your Strip Ready

- In my code I set up everything for exactly 30 LEDs, but you can change this setting if you want.
- Cut only at the marked copper pads (there are little scissor icons).


---

## 5. ESP32 Wiring {#esp32-wiring}

### The Hookup

It's pretty straightforward:

```
LED Strip          ESP32 Board
─────────────────────────────────
+5V (Red)    →    5V or VIN pin
GND (Black)  →    GND pin
DIN (Green)  →    GPIO 16
```

### Step by Step

**Power (Red Wire)**
- Find the **5V** or **VIN** pin on your ESP32
- This powers the LEDs straight from USB
- Don't use 3.3V - won't work

**Ground (Black Wire)**
- Any **GND** pin works (ESP32 has several)
- Just pick the closest one

**Data (Green/Yellow Wire)**
- Goes to **GPIO 16**
- That's what the code expects
- You can change this later in the firmware if you want


### Important Stuff

⚠️ **About Power**
The firmware caps LED power at 850mA. This is intentional - it keeps the device safe to run off any USB port without burning anything down. Don't mess with this unless you know what you're doing.

⚠️ **Data Line Tips**
- Keep the data wire short (under 20cm if possible)
- Don't run it right next to the power wires
- If you're feeling fancy, add a 330Ω resistor inline with the data wire for extra protection

---

## 6. Physical Assembly {#physical-assembly}

### Putting It All Together

**Install the LED Strip**
- Drop the strip into the LED holder piece
- Thread the wires through any cable channels
- Secure with hot glue or double-sided tape (your choice)
- Make sure the LEDs face outward toward where the diffuser will go

**Mount the ESP32**
- Place it in the base
- Line up the USB port with the enclosure opening (you need to plug this thing in)
- Stick it down with:
  - Hot glue on the corners, or
  - Double-sided foam tape, or
  - Whatever mounting solution your 3D print has

**Clean Up Those Wires**
- Route them nicely inside the base
- Don't bend the LED strip too sharply
- Make sure nothing's blocking the USB port
- Small cable ties work great here

**Close It Up**
- Snap the LED holder onto the base
- Power it on and make sure the LEDs work
- If everything looks good, put the diffuser on top
- Done

### Pro Tips
- Leave some slack in the wires so you can open it up later
- Coil any extra wire length in the base
- Double check you can still unplug the USB cable easily

---

## 7. Installing Required Libraries {#installing-libraries}

Assuming you already have Arduino IDE set up with ESP32 board support (if not, there are a million tutorials online for that), let's get the libraries installed.

### The Easy Ones (Library Manager)

Open the Library Manager in Arduino IDE (Sketch → Include Library → Manage Libraries) and search for each of these. Hit install for:

- **FastLED** by Daniel Garcia
- **ArduinoJson** by Benoit Blanchon (get version 6.x)
- **WiFiManager** by tzapu (the one with the most downloads)
- **UniversalTelegramBot** by Brian Lough
- **NTPClient** by Fabrice Weinberg

Each one: type the name, find it, click Install, accept dependencies if it asks.

### TinyExpr (The Manual One)

This one's not in the Library Manager, so we do it the old school way:

**Get the Files**
- Head to https://github.com/codeplea/tinyexpr
- Click the green "Code" button → Download ZIP
- Unzip it somewhere

**Copy to Arduino**
- You only need two files: `tinyexpr.h` and `tinyexpr.c`
- Find your Arduino libraries folder:
  - In Arduino IDE: File → Preferences
  - Look at "Sketchbook location"
  - Go to that folder, then into the `libraries` subfolder
- Create a new folder called `tinyexpr`
- Drop both files in there
- Restart Arduino IDE

**Check It Worked**
- Go to Sketch → Include Library
- You should see tinyexpr in the list somewhere
- If it's there, you're good

---

## 8. Flashing the Firmware {#flashing-firmware}

### Get the Code Ready

Grab the `.ino` file from this repo and open it in Arduino IDE.

### Configure Your Board

Go to the Tools menu and set everything up like this:

```
Board: "ESP32 Dev Module"
Upload Speed: "115200"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
```

That Partition Scheme thing is **critical**. If you don't set it to "Huge APP", you won't have enough space for the config files and the whole thing will fail. Don't skip this.

### Upload It

**Plug In Your ESP32**
Connect it to your computer with USB. Windows might need a second to install drivers.

**Select the Port**
Tools → Port, then pick whichever one shows your ESP32. On Windows it's usually COM3 or COM4. On Mac it's something like `/dev/cu.usbserial-xxxxx`.

**Verify First**
Click the checkmark button (or Ctrl+R) to make sure the code compiles. Fix any errors before moving on.

**Upload**
Hit the arrow button (or Ctrl+U). This takes about 30-60 seconds. You'll see a progress bar and a bunch of text flying by.

If it says "Connecting....." and nothing happens, try holding the BOOT button on your ESP32 while you click upload. Some boards are finicky like that.

### First Boot

After upload:
- All 30 LEDs should light up blue (that's the loading animation)
- Then the device creates a WiFi network called **SurfBeacon_Setup**
- If you see blue LEDs, you're good to go

---

## 9. Troubleshooting {#troubleshooting}

### Upload Problems

**"Failed to connect to ESP32"**
- Hold down the BOOT button while clicking upload
- Try a different USB cable (some only do power, not data)
- Your computer might need the CP2102 or CH340 drivers
- Lower the upload speed to 115200 in Tools menu

**"Sketch too big"**
You forgot to set the Partition Scheme to "Huge APP". Go back and fix that.

**"Port not found"**
- Install the USB drivers for your board (CP2102 or CH340)
- Try a different USB port
- Check Device Manager on Windows or run `ls /dev/tty*` on Mac/Linux

### LED Issues

**Nothing Lights Up**
- Check your wiring - is everything connected?
- Is the LED strip getting 5V power?
- Did you connect to DIN and not DOUT?
- Try uploading a simple FastLED example to test

**Some LEDs Don't Work**
- Look for breaks in the strip
- If one LED dies, everything after it goes dark
- Try changing `NUM_LEDS` in the code to isolate the problem

**Flickering or Wrong Colors**
- Make sure the data wire is secure on GPIO 16
- Check that GND is connected between ESP32 and strip
- Add a 330Ω resistor on the data line
- Try a big capacitor across the LED power lines

### WiFi Problems

**Can't See "SurfBeacon_Setup"**
- Wait 30 seconds after powering on
- Hit the reset button on the ESP32
- Re-flash the firmware
- Check the serial monitor for errors

**Won't Connect to Home WiFi**
- ESP32 only does 2.4GHz WiFi, not 5GHz
- Double check your password
- Move closer to the router
- Disable MAC filtering temporarily

### General Debugging

Open the Serial Monitor (Tools → Serial Monitor, set to 115200 baud) and watch what the device is saying. It prints startup info and errors there.

If everything's broken and you want to start over, just re-flash the firmware. That'll wipe the config and start fresh.

---

## Next Steps

Hardware done? LEDs showing blue? WiFi network appearing? Cool, now go read the Software Setup Guide to actually configure this thing.

---

## Safety Stuff

Don't be dumb:
- Stick to 5V power
- Don't block ventilation
- Keep it away from water
- Use decent USB cables and chargers

**Happy building! 🤙**
