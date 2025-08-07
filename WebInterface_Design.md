# WebInterface Module Documentation
## SkullStepperV4 - Phase 5 (IMPLEMENTED)

### Table of Contents
1. [Module Overview](#module-overview)
2. [Architecture Integration](#architecture-integration)
3. [Core Assignment and Threading](#core-assignment-and-threading)
4. [Inter-Module Communication](#inter-module-communication)
5. [Network Architecture](#network-architecture)
6. [Web Application Design](#web-application-design)
7. [Resource Management](#resource-management)
8. [Security Considerations](#security-considerations)
9. [Implementation Phases](#implementation-phases)
10. [API Specification](#api-specification)

---

## Module Overview

### Purpose
The WebInterface module provides wireless configuration and monitoring capabilities through a web-based interface, complementing the existing SerialInterface module. It creates a WiFi access point allowing users to connect via smartphone, tablet, or computer to control and configure the stepper system without physical serial connection.

**Status**: FULLY IMPLEMENTED and integrated into the production system.

### Key Features
- **WiFi Access Point** with configurable SSID/password
- **Real-time Status Dashboard** with WebSocket updates
- **Motion Control Interface** with safety interlocks
- **Configuration Management** with visual parameter editing
- **Responsive Design** for mobile and desktop devices
- **Zero External Dependencies** - all assets embedded

### Design Principles
- **Complete Module Isolation**: No direct calls to other modules
- **Thread-Safe Communication**: Uses existing GlobalInfrastructure
- **Non-Blocking Operation**: Async web server architecture
- **Minimal Resource Impact**: Optimized for embedded environment
- **Consistent Architecture**: Follows established module patterns

---

## Architecture Integration

### Module Placement
```
Core 0 (Real-Time)          Core 1 (Communication)
├── StepperController       ├── SerialInterface
├── SafetyMonitor          ├── SystemConfig
└── DMXReceiver            ├── WebInterface (NEW)
                           └── Main Loop

Communication via GlobalInfrastructure:
- g_motionCommandQueue
- g_systemStatus (protected)
- g_systemConfig (protected)
```

### Communication Rules
1. **No Direct Module Calls**: WebInterface only includes `GlobalInterface.h`
2. **Queue-Based Commands**: Motion commands sent via `g_motionCommandQueue`
3. **Protected Status Access**: Uses `SAFE_READ_STATUS` macros
4. **Configuration via SystemConfig**: Uses existing config manager methods

### Module Responsibilities
- **Network Management**: WiFi AP setup and client handling
- **HTTP/WebSocket Server**: Async request processing
- **Status Broadcasting**: Real-time updates via WebSocket
- **Command Translation**: Web requests → motion commands
- **Asset Serving**: Embedded HTML/CSS/JS delivery

---

## Core Assignment and Threading

### Core 1 Assignment (Mandatory)
WebInterface MUST run on Core 1 for the following reasons:
- Network operations are I/O heavy (perfect for Core 1)
- Shares core with other communication modules
- Avoids impacting real-time operations on Core 0
- Natural fit with SerialInterface pattern

### Task Structure
```cpp
class WebInterface {
private:
    TaskHandle_t webTaskHandle;
    TaskHandle_t broadcastTaskHandle;
    
    // Main web server task - handles HTTP/WebSocket
    static void webServerTask(void* parameter);
    
    // Status broadcast task - sends periodic updates
    static void statusBroadcastTask(void* parameter);
    
public:
    void begin() {
        // Create tasks pinned to Core 1
        xTaskCreatePinnedToCore(
            webServerTask,
            "WebServer",
            8192,           // Larger stack for web operations
            this,
            1,              // Lower priority than real-time
            &webTaskHandle,
            1               // Core 1 - CRITICAL!
        );
        
        xTaskCreatePinnedToCore(
            statusBroadcastTask,
            "WebBroadcast",
            4096,
            this,
            1,
            &broadcastTaskHandle,
            1               // Core 1
        );
    }
};
```

### Thread Safety
- **Mutex Protection**: All shared resource access protected
- **Lock-Free Status Reading**: Uses existing atomic read macros
- **Queue Timeouts**: Non-blocking command sends (timeout = 0)
- **No Blocking Operations**: Async handlers prevent deadlocks

---

## Inter-Module Communication

### Reading System Status
```cpp
// WebInterface status reading pattern
void WebInterface::getSystemStatus(JsonDocument& doc) {
    // Use existing thread-safe macros
    int32_t currentPos, targetPos;
    float currentSpeed, maxSpeed, acceleration;
    bool stepperEnabled;
    SystemState state;
    
    SAFE_READ_STATUS(currentPosition, currentPos);
    SAFE_READ_STATUS(targetPosition, targetPos);
    SAFE_READ_STATUS(currentSpeed, currentSpeed);
    SAFE_READ_STATUS(stepperEnabled, stepperEnabled);
    SAFE_READ_STATUS(systemState, state);
    
    // Read config through SystemConfig
    SystemConfig* config = SystemConfigMgr::getConfig();
    maxSpeed = config->motionProfile.maxSpeed;
    acceleration = config->motionProfile.acceleration;
    
    // Build JSON response
    doc["position"]["current"] = currentPos;
    doc["position"]["target"] = targetPos;
    doc["speed"]["current"] = currentSpeed;
    doc["speed"]["max"] = maxSpeed;
    doc["acceleration"] = acceleration;
    doc["state"] = static_cast<int>(state);
    doc["enabled"] = stepperEnabled;
}
```

### Sending Motion Commands
```cpp
// WebInterface motion command pattern
bool WebInterface::sendMotionCommand(CommandType type, int32_t position = 0) {
    MotionCommand cmd;
    cmd.type = type;
    cmd.timestamp = millis();
    cmd.commandId = nextCommandId++;
    
    if (type == CommandType::MOVE_ABSOLUTE) {
        cmd.profile.targetPosition = position;
        cmd.profile.moveType = MoveType::ABSOLUTE;
    }
    
    // Non-blocking send to avoid web server stalls
    if (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
        return true;
    }
    return false;  // Queue full
}
```

### Configuration Updates
```cpp
// WebInterface config update pattern
bool WebInterface::updateConfiguration(const JsonDocument& params) {
    bool success = true;
    
    // Use SystemConfig methods (thread-safe internally)
    if (params.containsKey("maxSpeed")) {
        float speed = params["maxSpeed"];
        if (!SystemConfigMgr::setMaxSpeed(speed)) {
            success = false;
        }
    }
    
    if (params.containsKey("acceleration")) {
        float accel = params["acceleration"];
        if (!SystemConfigMgr::setAcceleration(accel)) {
            success = false;
        }
    }
    
    // Commit changes to flash
    if (success) {
        SystemConfigMgr::commitChanges();
    }
    
    return success;
}
```

---

## Network Architecture

### Access Point Configuration
```cpp
// Configurable through SystemConfig
struct WebConfig {
    char apSSID[33] = "SkullStepper";
    char apPassword[65] = "";  // Empty = open network
    uint8_t apChannel = 6;
    bool apHidden = false;
    uint8_t maxClients = 2;
    IPAddress apIP = IPAddress(192, 168, 4, 1);
    IPAddress apGateway = IPAddress(192, 168, 4, 1);
    IPAddress apSubnet = IPAddress(255, 255, 255, 0);
};
```

### Network Stack
- **WiFi Mode**: Access Point only (no STA mode complexity)
- **IP Configuration**: Static IP for predictable access
- **DNS**: Optional captive portal for easy connection
- **mDNS**: Optional hostname resolution (skullstepper.local)

### Connection Management
```cpp
class WebInterface {
private:
    static constexpr uint8_t MAX_WS_CLIENTS = 2;
    AsyncWebSocket ws("/ws");
    uint8_t activeClients = 0;
    
    void onWsEvent(AsyncWebSocket* server, 
                   AsyncWebSocketClient* client,
                   AwsEventType type,
                   void* arg,
                   uint8_t* data,
                   size_t len) {
        switch(type) {
            case WS_EVT_CONNECT:
                if (activeClients >= MAX_WS_CLIENTS) {
                    client->close(1013, "Try again later");
                    return;
                }
                activeClients++;
                // Send initial status
                sendStatusUpdate(client);
                break;
                
            case WS_EVT_DISCONNECT:
                activeClients--;
                break;
                
            case WS_EVT_DATA:
                handleWsMessage(client, data, len);
                break;
        }
    }
};
```

---

## Web Application Design

### Asset Management
```cpp
// All assets embedded in PROGMEM to avoid filesystem dependency
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SkullStepper Control</title>
    <style>
        /* Embedded CSS - minified in production */
        :root {
            --primary-color: #00d4ff;
            --bg-color: #0a0a0a;
            --panel-bg: #1a1a1a;
            --text-color: #ffffff;
            --success: #00ff00;
            --warning: #ffaa00;
            --danger: #ff0044;
        }
        /* ... responsive design CSS ... */
    </style>
</head>
<body>
    <div id="app">
        <!-- Vue.js or vanilla JS app -->
    </div>
    <script>
        // Embedded JavaScript - minified in production
        const ws = new WebSocket(`ws://${window.location.host}/ws`);
        // ... application logic ...
    </script>
</body>
</html>
)rawliteral";
```

### UI Components

#### Status Dashboard
- **Position Display**: Current/Target with progress bar
- **Speed Indicator**: Real-time speed with gauge
- **State Display**: Visual state machine status
- **Limit Indicators**: Red/Green limit switch status
- **Alarm Status**: CL57Y alarm state display

#### Motion Control Panel
```javascript
// Motion control interface design
const MotionControl = {
    // Jog controls with safety
    jogButtons: {
        left: { step: -10, -100, -1000 },
        right: { step: +10, +100, +1000 }
    },
    
    // Absolute positioning
    goToPosition: {
        input: "number",
        min: "minPosition",
        max: "maxPosition",
        submit: "MOVE"
    },
    
    // Quick actions
    quickActions: [
        { label: "Home", command: "HOME", confirm: true },
        { label: "Stop", command: "STOP", class: "warning" },
        { label: "E-Stop", command: "ESTOP", class: "danger" }
    ]
};
```

#### Configuration Editor
- **Motion & Limits Tab**: All motion-related parameters including basic speeds, position limits, and advanced settings (jerk, emergency deceleration)
- **DMX Tab**: All DMX-related settings including channel, scale, offset, and timeout
- **Single Homing Warning**: Consolidated message stating configuration can be changed but movement requires homing
- **Save/Load/Reset**: Config management buttons

### Responsive Design
```css
/* Mobile-first responsive design */
.control-panel {
    display: grid;
    grid-template-columns: 1fr;
    gap: 1rem;
}

@media (min-width: 768px) {
    .control-panel {
        grid-template-columns: repeat(2, 1fr);
    }
}

@media (min-width: 1024px) {
    .control-panel {
        grid-template-columns: repeat(3, 1fr);
    }
}
```

---

## Resource Management

### Memory Budget
```cpp
// Estimated memory usage
class WebInterface {
    // Static allocations
    static constexpr size_t WEB_SERVER_TASK_STACK = 8192;  // 8KB
    static constexpr size_t BROADCAST_TASK_STACK = 4096;   // 4KB
    static constexpr size_t JSON_BUFFER_SIZE = 2048;       // 2KB
    static constexpr size_t WS_BUFFER_SIZE = 1024;         // 1KB per client
    
    // Dynamic allocations (per client)
    // AsyncWebServer: ~20KB per connection
    // WebSocket: ~15KB per connection
    // Total per client: ~35KB
    
    // Maximum memory with 2 clients: ~82KB
};
```

### CPU Usage Optimization
```cpp
// Broadcast task optimization
void WebInterface::statusBroadcastTask(void* parameter) {
    WebInterface* self = static_cast<WebInterface*>(parameter);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);  // 10Hz updates
    
    while (true) {
        if (self->activeClients > 0) {
            // Only build and send status if clients connected
            StaticJsonDocument<512> doc;
            self->getSystemStatus(doc);
            self->broadcastStatus(doc);
        }
        
        // Strict timing to prevent drift
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
```

### Network Optimization
- **Compression**: Gzip HTML/CSS/JS assets
- **Caching**: ETags for static resources
- **Throttling**: Rate limit WebSocket updates
- **Chunking**: Stream large responses

---

## Security Considerations

### Access Control
```cpp
struct WebSecurity {
    bool enableAuth = false;
    char username[33] = "admin";
    char password[65] = "";
    uint8_t maxLoginAttempts = 3;
    uint32_t lockoutTime = 300000;  // 5 minutes
};
```

### Security Features
1. **Optional Authentication**: Basic auth for protection
2. **Rate Limiting**: Prevent command flooding
3. **Input Validation**: All parameters validated
4. **CORS Policy**: Restrict cross-origin requests
5. **Secure Defaults**: No open network by default

### Command Validation
```cpp
bool WebInterface::validateCommand(const JsonDocument& cmd) {
    // Enforce same validation as SerialInterface
    if (!cmd.containsKey("command")) {
        return false;
    }
    
    String command = cmd["command"];
    
    // Check system state for safety
    SystemState state;
    SAFE_READ_STATUS(systemState, state);
    
    if (state == SystemState::EMERGENCY) {
        // Only allow ESTOP clear commands
        return (command == "clear_emergency");
    }
    
    // Validate parameters based on command
    if (command == "move") {
        if (!cmd.containsKey("position")) {
            return false;
        }
        int32_t pos = cmd["position"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        return (pos >= config->positionLimits.minPosition &&
                pos <= config->positionLimits.maxPosition);
    }
    
    return true;
}
```

---

## Implementation Status

### ✅ Phase 5.1: Core Infrastructure (COMPLETE)
- [x] Created WebInterface.h/cpp with module structure
- [x] Implemented Core 1 task creation
- [x] Set up ESP32 WebServer and WebSocketsServer
- [x] Created REST API endpoints
- [x] Tested coexistence with SerialInterface

### ✅ Phase 5.2: Web UI (COMPLETE)
- [x] Created embedded HTML/CSS/JS (no filesystem needed)
- [x] Implemented real-time status dashboard
- [x] Added WebSocket connection with auto-reconnect
- [x] Created complete motion controls
- [x] Real-time updates at 10Hz

### ✅ Phase 5.3: Configuration Management (COMPLETE)
- [x] Added configuration editor with tabs
- [x] Implemented parameter validation
- [x] Added save/apply functionality
- [x] Created motion, DMX, and limits configuration
- [x] Tested persistence to flash

### ✅ Phase 5.4: Advanced Features (COMPLETE)
- [x] Added jog controls (-1000 to +1000 steps)
- [x] Created TEST and TEST2 routine triggers
- [x] Added system state visualization
- [x] Implemented captive portal for easy connection
- [x] Added safety interlocks (homing required)

### ✅ Phase 5.5: Polish and Optimization (COMPLETE)
- [x] All assets embedded in PROGMEM
- [x] Optimized WebSocket updates (only when clients connected)
- [x] Added connection status indicators
- [x] Created responsive design for mobile
- [x] Performance tested with multiple clients

### ✅ Phase 5.6: UI Improvements (COMPLETE - v4.1.4-4.1.5)
- [x] Consolidated homing warnings into single message
- [x] Reorganized configuration tabs (removed Advanced tab)
- [x] Moved motion-related settings to Motion & Limits tab
- [x] Moved DMX timeout to DMX tab with other DMX settings
- [x] Allowed all configurations to be changed without homing
- [x] Fixed Auto-Home checkbox visibility - now always accessible

---

## API Specification

### REST Endpoints
```
GET  /              - Serve main HTML page
GET  /status        - Get current system status (JSON)
POST /command       - Send motion command
GET  /config        - Get configuration (JSON)
POST /config        - Update configuration
POST /config/reset  - Reset configuration
GET  /info          - System information
```

### WebSocket Protocol
```javascript
// Client → Server Messages
{
    "type": "command",
    "command": "move",
    "position": 1000
}

{
    "type": "config",
    "action": "set",
    "params": {
        "maxSpeed": 2000,
        "acceleration": 1500
    }
}

{
    "type": "subscribe",
    "topics": ["status", "config"]
}

// Server → Client Messages
{
    "type": "status",
    "data": {
        "position": { "current": 1000, "target": 1000 },
        "speed": { "current": 0, "max": 2000 },
        "state": "READY",
        "limits": { "left": false, "right": false }
    }
}

{
    "type": "error",
    "message": "Position out of range"
}

{
    "type": "success",
    "message": "Configuration updated"
}
```

### Example HTTP Requests
```bash
# Get status
curl http://192.168.4.1/status

# Send move command
curl -X POST http://192.168.4.1/command \
     -H "Content-Type: application/json" \
     -d '{"command":"move","position":1000}'

# Update configuration
curl -X POST http://192.168.4.1/config \
     -H "Content-Type: application/json" \
     -d '{"maxSpeed":2000,"acceleration":1500}'
```

---

## Implemented Libraries

### Current Implementation
1. **ESP32 WebServer** (Built-in with ESP32 core)
   - No additional installation required
   - Synchronous operation suitable for simple REST API
   - Lightweight and reliable
   - Part of ESP32 Arduino Core

2. **WebSocketsServer** (by Markus Sattler)
   - Repository: https://github.com/Links2004/arduinoWebSockets
   - Provides real-time WebSocket communication
   - Runs on separate port (81) from HTTP server
   - Install via Arduino Library Manager

3. **ArduinoJson** (JSON handling)
   - Repository: https://arduinojson.org/
   - Version: v6 or v7
   - Efficient JSON parsing/generation
   - Already used by SerialInterface

4. **Built-in ESP32 Libraries**
   - WiFi.h (ESP32 core WiFi functionality)
   - WebServer.h (HTTP server)
   - DNSServer.h (Captive portal support)

### Why This Implementation?
- **Maximum Compatibility**: Works with all ESP32 Arduino Core versions
- **Dual-server design**: HTTP (port 80) and WebSocket (port 81) separate
- **Simple and reliable**: Well-tested libraries
- **Minimal dependencies**: Only one external library needed
- **Thread-safe**: Properly integrated with FreeRTOS architecture

### Installation Instructions
```bash
# Arduino IDE Library Manager:
1. Tools → Manage Libraries
2. Search and install:
   - WebSockets by Markus Sattler
   - ArduinoJson by Benoit Blanchon (if not already installed)
```

## Module Integration Example

### WebInterface.h (Actual Implementation)
```cpp
#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "GlobalInterface.h"
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

class WebInterface {
public:
    static WebInterface& getInstance();
    
    void begin();
    void update();  // Called from main loop if needed
    bool isEnabled() const { return enabled; }
    uint8_t getClientCount() const { return activeClients; }
    String getAPAddress() const;
    
    // Configuration
    void setCredentials(const char* ssid, const char* password);
    void setEnabled(bool enable);
    
private:
    WebInterface();  // Singleton
    
    // Core components
    WebServer* httpServer;        // Port 80
    WebSocketsServer* wsServer;   // Port 81
    DNSServer* dnsServer;         // Captive portal
    
    // State
    bool enabled = true;
    bool running = false;
    volatile uint8_t activeClients;
    uint16_t nextCommandId = 1;
    
    // Tasks
    TaskHandle_t webTaskHandle = nullptr;
    TaskHandle_t broadcastTaskHandle = nullptr;
    
    // Task functions
    static void webServerTask(void* parameter);
    static void statusBroadcastTask(void* parameter);
    
    // HTTP handlers
    void handleRoot();
    void handleStatus();
    void handleCommand();
    void handleConfig();
    void handleConfigUpdate();
    void handleInfo();
    void handleCaptivePortal();
    
    // WebSocket handlers
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    void processWebSocketMessage(uint8_t num, uint8_t* payload, size_t length);
    
    // Internal methods
    void getSystemStatus(JsonDocument& doc);
    bool sendMotionCommand(CommandType type, int32_t position = 0);
    bool updateConfiguration(const JsonDocument& params);
    void broadcastStatus();
    bool validateCommand(const JsonDocument& cmd);
};

#endif // WEBINTERFACE_H
```

### Main Loop Integration
```cpp
// In skullstepperV4.ino
void setup() {
    // ... existing setup ...
    
    #ifdef ENABLE_WEB_INTERFACE
    // Initialize WebInterface on Core 1
    WebInterface::getInstance().begin();
    Serial.println("Web Interface started on http://192.168.4.1");
    #endif
}

void loop() {
    // ... existing loop code ...
    
    #ifdef ENABLE_WEB_INTERFACE
    // WebInterface runs in its own tasks, minimal loop overhead
    WebInterface::getInstance().update();
    #endif
}
```

---

## Testing Strategy

### Unit Tests
1. **Isolation Test**: Disable all other modules except WebInterface
2. **Command Queue Test**: Verify motion commands reach StepperController
3. **Status Reading Test**: Confirm thread-safe status access
4. **Memory Test**: Monitor heap usage under load
5. **Stress Test**: Multiple clients with rapid commands

### Integration Tests
1. **Dual Interface Test**: Serial and Web commands simultaneously
2. **State Consistency**: Both interfaces show same status
3. **Configuration Sync**: Changes via web appear in serial
4. **Performance Test**: Measure Core 0 impact
5. **Reliability Test**: 24-hour operation test

### Performance Metrics
- Web server response time: <50ms
- WebSocket latency: <10ms
- Status update rate: 10Hz
- CPU usage on Core 1: <20%
- Memory overhead: <100KB

---

## Conclusion

The WebInterface module successfully extends SkullStepperV4's capabilities while maintaining strict architectural compliance. By following the established patterns of complete module isolation, thread-safe communication, and proper core assignment, it has been seamlessly integrated without impacting real-time performance.

The dual-server implementation (HTTP on port 80, WebSocket on port 81) provides reliable operation with minimal dependencies. The embedded asset approach eliminates filesystem requirements, and careful resource management ensures the ESP32-S3 can handle this functionality alongside all other modules.

This implementation provides a professional-grade wireless interface that perfectly complements the existing SerialInterface, giving users flexible options for system control and monitoring. The system is now production-ready with both serial and web-based control interfaces.

---

*WebInterface Module Documentation v2.0.0*  
*SkullStepperV4 Phase 5 - IMPLEMENTED*