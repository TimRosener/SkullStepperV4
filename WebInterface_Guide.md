# WebInterface Installation Guide - WebServer + WebSockets

## Overview
This WebInterface implementation uses the built-in ESP32 WebServer library combined with WebSocketsServer for real-time updates. This approach is fully compatible with ESP32 Arduino Core 3.x.

## Required Library

### WebSockets
- **Author**: Markus Sattler
- **Install via Arduino IDE**:
  1. Open Arduino IDE
  2. Go to **Tools → Manage Libraries...**
  3. Search for: `WebSockets`
  4. Find **"WebSockets by Markus Sattler"**
  5. Click **Install**

That's it! Only one additional library needed (WebServer is built into ESP32 core).

## Architecture

- **HTTP Server**: Port 80 - Serves web interface and REST API
- **WebSocket Server**: Port 81 - Real-time bidirectional communication
- **No filesystem required** - All content served from memory
- **Thread-safe** - Properly integrated with FreeRTOS architecture
- **Dual-server design** - HTTP and WebSocket run independently

## Features

- **Real-time status display** - Position, speed, limits, motor state
- **Motion control** - Move, jog, home, stop, emergency stop
- **Motor enable/disable** - Control motor power
- **Configuration** - Adjust max speed and acceleration
- **Auto-reconnecting WebSocket** - Handles disconnections gracefully
- **Responsive design** - Works on phones, tablets, and computers
- **10Hz status updates** - Smooth real-time feedback

## Usage

1. **Upload the code** to your ESP32-S3
2. **Connect to WiFi**: Look for "SkullStepper" network (no password)
3. **Open browser**: Navigate to http://192.168.4.1
4. **Control your stepper**: Use the web interface!

## How It Works

1. **WebServer** (Port 80) handles:
   - Serving the main HTML/CSS/JS page
   - REST API endpoints for status and commands
   - Configuration updates

2. **WebSocketsServer** (Port 81) handles:
   - Real-time bidirectional communication
   - Status updates at 10Hz
   - Instant command transmission
   - Connection state management

## API Endpoints

### HTTP REST API (Port 80)
- `GET /` - Main web interface
- `GET /api/status` - Current system status (JSON)
- `POST /api/command` - Send motion command
- `GET /api/config` - Get configuration
- `POST /api/config` - Update configuration
- `GET /api/info` - System information

### WebSocket Protocol (Port 81)
All WebSocket messages use JSON format.

**Client → Server:**
```json
{"type": "command", "command": "move", "position": 1000}
{"type": "command", "command": "home"}
{"type": "getStatus"}
```

**Server → Client:**
```json
{
  "systemState": 2,
  "position": {"current": 1000, "target": 1000},
  "speed": 0.0,
  "stepperEnabled": true,
  "limits": {"left": false, "right": false}
}
```

## Troubleshooting

### Can't connect to WiFi
- Make sure WebInterface is enabled in ProjectConfig.h
- Check serial output for AP startup confirmation
- Try refreshing WiFi list on your device

### Web page won't load
- Verify IP address (should be 192.168.4.1)
- Check that port 80 is not blocked
- Try a different browser

### WebSocket won't connect
- WebSocket runs on port 81 (not 80)
- Check browser console for errors
- Ensure JavaScript is enabled

### No real-time updates
- WebSocket might be disconnected
- Check connection status indicator (top right)
- Page will auto-reconnect every 3 seconds

## Customization

All HTML, CSS, and JavaScript is embedded in WebInterface.cpp:
- `getIndexHTML()` - Main HTML structure
- `getMainCSS()` - Styling (dark theme by default)
- `getMainJS()` - Client-side logic with WebSocket handling

The interface is designed to be easily customizable while maintaining the thread-safe architecture.

## Performance

- HTTP requests are handled synchronously but quickly
- WebSocket operates asynchronously for real-time updates
- Status broadcasts only occur when clients are connected
- Minimal CPU usage on Core 1
- No impact on Core 0 real-time operations

## Advantages of This Approach

1. **Maximum Compatibility** - Works with any ESP32 Arduino Core version
2. **Simple Dependencies** - Only one external library needed
3. **Proven Reliability** - Built on mature, stable libraries
4. **Easy Debugging** - Standard HTTP/WebSocket protocols
5. **Flexible Architecture** - HTTP and WebSocket can be used independently