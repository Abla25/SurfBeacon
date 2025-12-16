# 💻 SurfBeacon - Software Setup & Configuration Guide

## Table of Contents
1. [Initial WiFi Setup](#wifi-setup)
2. [Accessing the Web Interface](#web-interface)
3. [Tab 1: WAVES - Your Surf Spots](#waves-tab)
4. [Tab 2: LAMP - Ambient Mode](#lamp-tab)
5. [Tab 3: CONFIG - System Settings](#config-tab)
6. [Setting Up Telegram](#telegram-setup)
7. [How Surf Scoring Actually Works](#surf-scoring)
8. [Pro Mode Explained](#pro-mode)
9. [Custom Formula Builder](#custom-formula)
10. [Daily Usage](#daily-usage)
11. [When Things Go Wrong](#troubleshooting)

---

## 1. Initial WiFi Setup {#wifi-setup}

### First Power-Up

You've built the thing, uploaded the firmware, and now you're staring at blue LEDs. Good start.

After about 10-15 seconds, the device creates its own WiFi network called **SurfBeacon_Setup**. No password needed.

### Getting Online

**Find the Network**
Open your phone's WiFi settings and connect to SurfBeacon_Setup. Most phones will automatically pop up a configuration page. If not, open a browser and go to `192.168.4.1`.

**Connect to Your WiFi**
Click "Configure WiFi", pick your home network from the list, type in the password (carefully), and hit Save. The device tries to connect for about 20 seconds. If it works, it reboots. If not, the setup network comes back and you try again.

### WiFi Quirks

The ESP32 only does 2.4GHz WiFi. If your router broadcasts both 2.4GHz and 5GHz on the same name, it should pick the right one automatically. But if you're having issues, create a separate 2.4GHz network name.

---

## 2. Accessing the Web Interface {#web-interface}

### Finding Your Device

Once it's connected to WiFi, open a browser and try:

**http://surfbeacon.local**

That works on most devices (Mac, modern Windows, phones). If it doesn't:

**Use the IP Address**
Check your router's connected devices list, find something called "ESP32" or similar, note the IP address (like `192.168.1.100`), and go to that in your browser.

The interface works on any browser, but Chrome/Edge have the best compatibility.

---

## 3. Tab 1: WAVES - Your Surf Spots {#waves-tab}

<img width="300" alt="Xnapper-2025-12-16-19 02 09" src="https://github.com/user-attachments/assets/b4e39a20-875e-4ac1-a1c4-38cfb4cadbad" />

This is where you set up the spots you care about.

### Adding a Spot

Hit "ADD NEW SPOT" and you'll get a new card with a database search tool built in.

**Using the Database** (Easiest Way)
The device has 3,000+ surf spots pre-loaded. Pick a country from the dropdown, start typing the spot name, and suggestions pop up. Click one and it auto-fills the coordinates.

**Manual Entry**
Or just type in:
- Spot Name (whatever you want to call it)
- Latitude (decimal format, like `36.7783`)
- Longitude (decimal format, like `-121.7967`)

Get coordinates from Google Maps: right-click on a location, copy the first two numbers.

### Spot Settings

**Enable/Disable Toggle**
Green means it's active and being monitored. Gray means it's ignored. Useful for travel spots you don't always care about.

**Forecast Days (1-7)**
How many days ahead to check. Default is 3. More days means better odds of finding good surf, but also more API calls.

**Color Picker**
What color the LEDs show when this spot is firing. Makes it easy to tell which spot is good when you have multiple.

### The Scoring Thresholds

**Min Score**
The minimum score needed to light up the lamp. Set this based on what actually works at your spot. Examples:
- Pacific Coast: 15-20
- Mediterranean: 5-10
- Big wave spot: 40-60

**Epic Score**
When things get really good. Usually 2-3x your Min Score. This triggers the epic animation and (depending on your settings) Telegram notifications.

**Reference Guide**
There's a little info tooltip (ℹ️) with suggested ranges:

Ocean spots (Hawaii, Indo): Fair > 10, Good > 30, Epic > 80
Open ocean (Europe, US): Fair > 5, Good > 15, Epic > 50
Seas (Med, Baltic): Fair > 2, Good > 6, Epic > 25

These are just starting points. Adjust based on reality.

**Max Score Display**
Each spot card shows the highest score in the forecast period. Updates hourly (or whatever you set the update frequency to).

---

## 4. Pro Mode {#pro-mode}

If your spot only works with certain swell directions or is super wind-sensitive, turn on Pro Mode.

<img width="300" alt="Xnapper-2025-12-16-19 03 57" src="https://github.com/user-attachments/assets/975aafd0-adf8-4520-bf44-107f0aace399" />

### When You Need It

Use Pro Mode when:
- Only specific swell directions produce waves (blocked by islands/headlands)
- Offshore wind makes all the difference
- Onshore wind destroys everything
- You want more control over the scoring

### Best Swell Directions

Click the compass grid buttons to select which directions work. Active directions light up purple. The device checks if the incoming swell is within ±45° of any selected direction. If not, score goes to zero.

Common patterns:
- Point breaks: 2-3 adjacent directions
- Beach breaks: 4-6 directions
- Reef breaks: 1-3 specific directions

### Ideal Offshore Wind

Pick which direction is offshore for your spot. This is the wind blowing from land to sea.

To figure it out: stand on the beach facing the water. Wind in your face = onshore (bad). Wind at your back = offshore (good). Select that direction.

### Wind Multipliers

When Pro Mode is on, the device automatically adjusts scores based on wind:

**Offshore Wind (within ±45° of your setting)**
- Under 15 km/h: Score × 1.3 (clean conditions)
- 15-30 km/h: Score × 1.1 (still good)
- Over 30 km/h: Score × 0.9 (getting blown out)

**Cross-shore Wind (45-135° off)**
- Under 12 km/h: Score × 0.9 (manageable)
- Over 12 km/h: Score × 0.6 (choppy)

**Onshore Wind (opposite direction)**
- Under 10 km/h: Score × 0.8 (not great)
- 10-20 km/h: Score × 0.4 (pretty bad)
- Over 20 km/h: Score × 0.2 (blown out)

---

## 5. Tab 2: LAMP - Ambient Mode {#lamp-tab}

When no spots are firing, the SurfBeacon can function as a regular lamp instead of just sitting there dark.

<img width="300" alt="Xnapper-2025-12-16-19 04 21" src="https://github.com/user-attachments/assets/6b16a5b8-8e99-42a2-95ad-65899057228f" />

### Fallback Settings

**Enable Switch**
Turn this on if you want lamp mode. Turn it off if you prefer the device to go dark when there's no surf.

**Animation Types**
- Static Light: Just a solid color
- Breathe (Pulse): Gentle breathing effect
- Tide (Flowing): Wave-like motion
- Coastal (Plasma): Complex water texture
- Swell (Gradient): Moving gradient
- Breaker (Comet): Shooting star effect
- Rainbow Cycle: Full spectrum rotation

**Color Picker**
Sets the lamp color (except for Rainbow Cycle where it's ignored). Default is a warm orange.

**Animation Speed (1-255)**
How fast the effect moves. Low numbers (20-60) are calm and meditative. High numbers (180-255) are more dynamic.

### Some Ideas

Bedroom nightlight: Breathe animation, warm white, speed 25
Office ambient: Coastal plasma, cyan blue, speed 80
Living room accent: Rainbow Cycle, speed 128

---

## 6. Tab 3: CONFIG - System Settings {#config-tab}

### LED Settings

**Master Brightness (5-255)**
Controls brightness for both surf mode and lamp mode. Default is 255 (full blast). Set it to 100-150 for nighttime use if it's too bright.

**Standard Conditions**
This is what happens when surf score is between your Min and Epic thresholds.

- Cycle Time: How long to show each firing spot (default 20 seconds)
- Animation: What effect to use (Breathe or Tide work well)
- Speed: Auto-calculated based on surf score. Near min threshold = slow and gentle. Near epic = faster and more energetic.

**Epic Conditions**
When a spot hits its Epic threshold:
- Animation: Separate effect (Rainbow Cycle is dramatic)
- Color: Pick a color (ignored for Rainbow)
- Effect Speed: Fixed speed, default 200 (fast)

### Night Mode (Do Not Disturb)

Turn LEDs completely off during set hours. Helps you actually sleep.

- Quiet Start: Hour to turn off (24-hour format, default 23 = 11 PM)
- Quiet End: Hour to turn back on (default 7 = 7 AM)

Examples:
- Night owl: Start 1, End 10 (1 AM to 10 AM off)
- Early riser: Start 22, End 6 (10 PM to 6 AM off)

<img width="300" height="704" alt="Xnapper-2025-12-16-19 05 20" src="https://github.com/user-attachments/assets/2a7f824e-c713-4150-a41a-95c4a289bb72" />


### System Settings

**Update Frequency**
How often to check weather:
- 30 mins (frequent, good for dawn patrol)
- 1 hour (balanced, recommended)
- 2-12 hours (less data usage)
- 24 hours (minimal)

**Time Zone**
Your GMT offset (GMT-12 to GMT+14). Makes sure logs show correct local time. The device uses GMT internally.

**System Log**
Shows the last 40 events with timestamps. Useful for debugging. Look here if something's not working right.

---

## 7. Setting Up Telegram {#telegram-setup}

Get notifications on your phone when surf's up.

### Create a Bot

Open Telegram (install it if you don't have it) and search for @BotFather. This is the official bot for creating bots.

Send it: `/newbot`

It asks for a name: `MySurfBot` (or whatever)
Then a username: `mysurfbeacon_bot` (must end in 'bot')

BotFather replies with your Bot Token. Looks like: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

Keep this secret.

### Get Your Chat ID

Start a chat with your new bot (click the link BotFather gave you) and send it any message.

Then open a browser and go to:
```
https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
```
Replace `<YOUR_BOT_TOKEN>` with your actual token.

Look for `"chat":{"id":123456789` - that number is your Chat ID.

Alternative: Use @userinfobot in Telegram. Send it a message and it tells you your Chat ID.

### Configure in SurfBeacon

In the CONFIG tab:

- Toggle Telegram switch ON
- Paste your Bot Token
- Paste your Chat ID (just the numbers, no quotes)
- Choose when to notify:
  - **Any Good Surf**: Alerts when any spot exceeds its Min threshold (most frequent)
  - **Epic Surf Only**: Only when a spot hits Epic threshold (less spam)
  - **Custom Global Score**: Set one threshold that applies to all spots

### Notification Format

When conditions are good:
```
🏄‍♂️ SURF CALLING!

📍 Your Spot Name
📅 2025-12-16
🕒 Early Morning, Afternoon
🌊 Score: 45
😎 Looking good.
```

Or for epic:
```
🏄‍♂️ SURF CALLING!

📍 Epic Reef
📅 2025-12-16
🕒 Late Morning, Afternoon
🌊 Score: 85
🔥 EPIC CONDITIONS!
```

### Smart Notifications

The device remembers what it already notified you about. It won't spam you if conditions stay good. You only get a new notification if:
- Different day
- Score changes significantly
- Conditions dropped then came back up

---

## 8. How Surf Scoring Actually Works {#surf-scoring}

### Standard Mode (Pro Mode Off)

Simple energy calculation:
```
Score = (Height² × Period) × 1.5
```

Example: 1.5m swell, 12s period
- Score = (1.5² × 12) × 1.5 = 40.5

Works pretty well for most spots. Doesn't account for wind or swell direction though.

### Advanced Mode (Pro Mode On)

When you enable Pro Mode:
```
Score = Energy × Wind Multiplier × Angle Factor × Chop Reduction
```

1. Calculate base energy (same formula)
2. Check if swell direction matches your selected compass directions (if not, score = 0)
3. Apply wind multiplier based on offshore/onshore/crossshore
4. Reduce score if wind waves are creating choppy conditions

Example (good conditions):
- 1.8m swell, 14s, from Northwest (selected)
- 8 km/h wind from East (offshore)
- Base energy: 68
- Offshore bonus: ×1.3
- Final: 88 (Epic!)

Example (poor conditions):
- 1.5m swell, 11s, from South (not selected)
- Score: 0 (blocked)

---

## 9. Custom Formula Builder {#custom-formula}

For when you want complete control.

### How It Works

In CONFIG tab, toggle "Pro Formula Builder" ON. This overrides ALL logic (both standard and pro mode) with your custom equation. Applies to every spot.

### Available Variables

- `h` = Swell height (meters)
- `p` = Swell period (seconds)
- `w` = Wind speed (km/h)
- `off` = Offshore factor (0.0 to 1.0, from pro mode settings)

### Operators

`+` `-` `*` `/` `()`

Plus functions like `sqrt()`, `pow()`, `abs()`.

### Example Formulas

Simple energy with wind penalty:
```
(h * h * p) * (1 - (w / 100))
```

Offshore-dependent:
```
h * p * off * 2
```

Period-weighted:
```
(h * h) * (p / 10) * off
```

<img width="300" alt="Xnapper-2025-12-16-19 05 43" src="https://github.com/user-attachments/assets/15a5a286-8110-4214-9aff-37f8b9ef7216" />

### Testing

After saving a formula, wait for the next update and check the Max Scores on your spots. If they look wrong, you probably have a syntax error or division by zero. The device falls back to standard mode if your formula breaks.

Use the calculator buttons or type directly in the field.

---

## 10. Daily Usage {#daily-usage}

### Reading the Lamp

**LED Color** = Which spot is firing (matches the color picker)
**Animation Speed** = How good it is (slow = marginal, fast = epic)
**Epic Mode** = Outstanding conditions (different animation, usually rainbow)
**Lamp Mode** = No spots firing (different color)
**Off** = Night mode active or globally disabled

### Morning Routine

1. Check lamp from bed - is it glowing?
2. Open web interface - see which spot and when
3. Check Telegram - if you got notified, definitely go

### Adjusting Thresholds

**Too many false positives?**
- Raise Min Threshold by 5-10 points
- Enable Pro Mode for wind filtering
- Reduce forecast days

**Missing good days?**
- Lower Min Threshold by 5-10 points
- Add more swell directions in Pro Mode
- Increase forecast days
- Verify coordinates are accurate

### Multi-Spot Strategy

Add 3-5 spots nearby with different colors. The lamp rotates through whatever's firing. Add travel spots too - just disable them when not needed.

---

## 11. When Things Go Wrong {#troubleshooting}

### Can't Access Web Interface

**http://surfbeacon.local doesn't work**
- Try the IP address instead (check your router)
- Flush DNS cache
- Try different browser or incognito mode
- Disable VPN

**Page loads but shows "Connecting..."**
- Device might be off
- Hit reset button on ESP32
- Wait 30 seconds then refresh

**Changes don't save**
- Click "SAVE CHANGES" button at bottom
- Wait for "REBOOTING..." message
- Don't close browser during save

### Forecast Issues

**All scores show zero**
- Pro Mode might be too restrictive (try disabling it)
- Coordinates might be wrong (verify in Google Maps)
- Check system log for API errors
- Might genuinely be flat

**Scores seem off**
- Compare with real conditions over a week
- Adjust thresholds
- Try Pro Mode or Custom Formula

### Telegram Problems

**Not getting notifications**
- Check toggle is ON
- Verify Bot Token (no spaces, full string)
- Verify Chat ID (just numbers)
- Make sure you messaged the bot first
- Check system log for errors
- Test token at: `https://api.telegram.org/bot<TOKEN>/getMe`

**Too many notifications**
- Switch to "Epic Surf Only"
- Raise Custom Global Score threshold
- Reduce forecast days

### LED Behavior

**Stuck on blue**
- Device stuck in boot loop (WiFi issue)
- Hold reset for 10 seconds
- Reconfigure WiFi via setup portal

**Turn off during the day**
- Check Night Mode hours
- Check master power toggle (icon next to "SURF BEACON")

**Wrong colors**
- Reset color picker and save again

### General Debugging

Open Serial Monitor in Arduino IDE (115200 baud) and watch the device talk to you. Re-flash firmware to factory reset.

---

## Quick Reference

**Web Interface:** http://surfbeacon.local

**Beginner Setup:**
- 1 spot, Pro Mode off
- Min: 10-15, Epic: 40-50
- Update: 1 hour
- Breathe animation

**Advanced Setup:**
- 3-5 spots, Pro Mode on
- Custom thresholds per spot
- Telegram enabled
- 20s cycle time

---

**Now go check if it's firing. 🏄‍♂️**
