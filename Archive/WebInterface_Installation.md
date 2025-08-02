# WebInterface Library Installation Guide

## Required Libraries

The WebInterface module requires two libraries that must be installed via the Arduino Library Manager:

### 1. AsyncTCP (MUST INSTALL FIRST)
- **Author**: me-no-dev
- **Purpose**: Async TCP library for ESP32
- **Required by**: ESPAsyncWebServer

### 2. ESPAsyncWebServer
- **Author**: me-no-dev  
- **Purpose**: Async HTTP and WebSocket server
- **Depends on**: AsyncTCP

## Installation Steps

### Method 1: Arduino IDE Library Manager (Recommended)

1. Open Arduino IDE
2. Go to **Tools → Manage Libraries...**
3. In the search box, type `AsyncTCP`
4. Find **"AsyncTCP by me-no-dev"**
5. Click **Install**
6. In the search box, type `ESPAsyncWebServer`
7. Find **"ESP Async WebServer by me-no-dev"**
8. Click **Install**

### Method 2: Manual Installation

If the Library Manager doesn't work, you can install manually:

1. **Download AsyncTCP**:
   - Go to: https://github.com/me-no-dev/AsyncTCP
   - Click "Code" → "Download ZIP"
   - In Arduino IDE: Sketch → Include Library → Add .ZIP Library
   - Select the downloaded AsyncTCP-master.zip

2. **Download ESPAsyncWebServer**:
   - Go to: https://github.com/me-no-dev/ESPAsyncWebServer
   - Click "Code" → "Download ZIP"
   - In Arduino IDE: Sketch → Include Library → Add .ZIP Library
   - Select the downloaded ESPAsyncWebServer-master.zip

## Verify Installation

After installation, you should be able to compile the SkullStepperV4 project with WebInterface enabled.

To test:
1. Make sure `#define ENABLE_WEB_INTERFACE` is uncommented in skullstepperV4.ino
2. Compile the project
3. If successful, the WebInterface will be available after upload

## Troubleshooting

### "AsyncTCP.h: No such file or directory"
- AsyncTCP is not installed
- Install AsyncTCP first, then ESPAsyncWebServer

### "Multiple libraries were found for..."
- You may have duplicate installations
- Remove old versions from your Arduino/libraries folder

### Library conflicts
- Make sure you're using the ESP32 board package (not ESP8266)
- Board: "ESP32S3 Dev Module" or similar ESP32-S3 board

## Usage

Once libraries are installed:
1. Upload the SkullStepperV4 code
2. Connect to WiFi network "SkullStepper" (no password)
3. Open web browser to http://192.168.4.1
4. Enjoy wireless control!

## Disabling WebInterface

If you don't want to use the WebInterface:
- Comment out `#define ENABLE_WEB_INTERFACE` in skullstepperV4.ino
- The project will compile without requiring these libraries