# WebInterface PsychicHttp Installation Guide

## Overview
This is the new WebInterface implementation using PsychicHttp, which is fully compatible with ESP32 Arduino Core 3.x.

## Required Library

### PsychicHttp
- **Author**: hoeken
- **Install via Arduino IDE**:
  1. Open Arduino IDE
  2. Go to **Tools → Manage Libraries...**
  3. Search for: `PsychicHttp`
  4. Find **"PsychicHttp by hoeken"**
  5. Click **Install**

That's it! Only one library needed (ArduinoJson should already be installed).

## Features

- **No filesystem required** - All content served from PROGMEM
- **WebSocket support** - Real-time updates at 10Hz
- **Responsive design** - Works on mobile and desktop
- **Thread-safe** - Properly integrated with FreeRTOS architecture
- **ESP32-S3 optimized** - Uses native ESP-IDF HTTP server

## Usage

1. **Upload the code** to your ESP32-S3
2. **Connect to WiFi**: Look for "SkullStepper" network (no password)
3. **Open browser**: Navigate to http://192.168.4.1
4. **Control your stepper**: Use the web interface!

## Web Interface Features

- **Real-time status display** - Position, speed, limits, motor state
- **Motion control** - Move, jog, home, stop, emergency stop
- **Motor enable/disable** - Control motor power
- **Configuration** - Adjust max speed and acceleration
- **WebSocket connection** - Automatic reconnection
- **Responsive design** - Works on phones, tablets, and computers

## Troubleshooting

### Can't connect to WiFi
- Make sure WebInterface is enabled in ProjectConfig.h
- Check serial output for AP startup confirmation
- Try refreshing WiFi list on your device

### Web page won't load
- Verify IP address (should be 192.168.4.1)
- Check that only one device is connected (max 4 connections)
- Try a different browser

### WebSocket disconnects
- This is normal when switching tabs/apps
- It will automatically reconnect when you return
- Check the connection status indicator in the top right

## Customization

All HTML, CSS, and JavaScript is embedded in WebInterface.cpp:
- `getIndexHTML()` - Main HTML structure
- `getMainCSS()` - Styling (dark theme by default)
- `getMainJS()` - Client-side logic

You can modify these functions to customize the interface.