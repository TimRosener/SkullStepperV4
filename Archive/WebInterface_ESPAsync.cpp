/*
 * WebInterface.cpp
 * 
 * Implementation of WiFi web interface for SkullStepperV4
 * 
 * This module provides:
 * - WiFi Access Point for direct connection
 * - Async web server with REST API
 * - WebSocket for real-time updates
 * - Responsive web UI for control and configuration
 */

#include "ProjectConfig.h"  // Get feature switches
#include "WebInterface.h"

#ifdef ENABLE_WEB_INTERFACE  // Only compile if web interface is enabled

// HTML content stored in PROGMEM
const char WebInterface::index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SkullStepper Control</title>
    <style>
        :root {
            --primary: #00d4ff;
            --bg: #0a0a0a;
            --panel: #1a1a1a;
            --text: #ffffff;
            --success: #00ff00;
            --warning: #ffaa00;
            --danger: #ff0044;
            --border: #333;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg);
            color: var(--text);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        h1 {
            color: var(--primary);
            margin-bottom: 30px;
            text-align: center;
            font-size: 2.5em;
            text-shadow: 0 0 20px rgba(0, 212, 255, 0.5);
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .panel {
            background: var(--panel);
            border: 1px solid var(--border);
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
        }
        
        .panel h2 {
            color: var(--primary);
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        
        .status-item {
            display: flex;
            justify-content: space-between;
            margin-bottom: 10px;
            padding: 5px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }
        
        .status-value {
            font-weight: bold;
            color: var(--primary);
        }
        
        .control-buttons {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 10px;
            margin-top: 20px;
        }
        
        button {
            background: var(--primary);
            color: var(--bg);
            border: none;
            padding: 12px 20px;
            border-radius: 5px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 212, 255, 0.4);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        button.danger {
            background: var(--danger);
        }
        
        button.warning {
            background: var(--warning);
        }
        
        button:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        
        .position-control {
            margin-top: 20px;
        }
        
        input[type="number"] {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            background: var(--bg);
            border: 1px solid var(--border);
            color: var(--text);
            border-radius: 5px;
            font-size: 16px;
        }
        
        input[type="range"] {
            width: 100%;
            margin: 10px 0;
        }
        
        .slider-value {
            text-align: center;
            color: var(--primary);
            font-weight: bold;
            margin-bottom: 5px;
        }
        
        .connection-status {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 10px 20px;
            background: var(--panel);
            border-radius: 20px;
            border: 2px solid var(--success);
        }
        
        .connection-status.disconnected {
            border-color: var(--danger);
        }
        
        .limit-indicator {
            display: inline-block;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            margin-left: 10px;
        }
        
        .limit-indicator.active {
            background: var(--danger);
            box-shadow: 0 0 10px var(--danger);
        }
        
        .limit-indicator.inactive {
            background: var(--success);
            box-shadow: 0 0 10px var(--success);
        }
        
        @media (max-width: 768px) {
            h1 { font-size: 2em; }
            .panel { padding: 15px; }
            button { padding: 10px 15px; font-size: 14px; }
        }
    </style>
</head>
<body>
    <div class="connection-status" id="connectionStatus">
        <span id="connectionText">Connecting...</span>
    </div>
    
    <div class="container">
        <h1>SkullStepper Control</h1>
        
        <div class="status-grid">
            <div class="panel">
                <h2>Position</h2>
                <div class="status-item">
                    <span>Current</span>
                    <span class="status-value" id="currentPos">-</span>
                </div>
                <div class="status-item">
                    <span>Target</span>
                    <span class="status-value" id="targetPos">-</span>
                </div>
                <div class="status-item">
                    <span>Speed</span>
                    <span class="status-value" id="currentSpeed">-</span>
                </div>
            </div>
            
            <div class="panel">
                <h2>System Status</h2>
                <div class="status-item">
                    <span>State</span>
                    <span class="status-value" id="systemState">-</span>
                </div>
                <div class="status-item">
                    <span>Motor</span>
                    <span class="status-value" id="motorState">-</span>
                </div>
                <div class="status-item">
                    <span>Limits</span>
                    <span>
                        L<span class="limit-indicator" id="leftLimit"></span>
                        R<span class="limit-indicator" id="rightLimit"></span>
                    </span>
                </div>
            </div>
            
            <div class="panel">
                <h2>Configuration</h2>
                <div class="status-item">
                    <span>Max Speed</span>
                    <span class="status-value" id="maxSpeed">-</span>
                </div>
                <div class="status-item">
                    <span>Acceleration</span>
                    <span class="status-value" id="acceleration">-</span>
                </div>
                <div class="status-item">
                    <span>DMX Channel</span>
                    <span class="status-value" id="dmxChannel">-</span>
                </div>
            </div>
        </div>
        
        <div class="panel">
            <h2>Motion Control</h2>
            <div class="control-buttons">
                <button onclick="sendCommand('home')">HOME</button>
                <button onclick="sendCommand('stop')" class="warning">STOP</button>
                <button onclick="sendCommand('estop')" class="danger">E-STOP</button>
                <button onclick="sendCommand('enable')" id="enableBtn">ENABLE</button>
                <button onclick="sendCommand('disable')" id="disableBtn">DISABLE</button>
            </div>
            
            <div class="position-control">
                <h3>Move to Position</h3>
                <input type="number" id="targetPosition" placeholder="Enter target position">
                <button onclick="moveToPosition()">MOVE</button>
            </div>
            
            <div class="position-control">
                <h3>Jog Control</h3>
                <div class="control-buttons">
                    <button onclick="jog(-1000)">-1000</button>
                    <button onclick="jog(-100)">-100</button>
                    <button onclick="jog(-10)">-10</button>
                    <button onclick="jog(10)">+10</button>
                    <button onclick="jog(100)">+100</button>
                    <button onclick="jog(1000)">+1000</button>
                </div>
            </div>
        </div>
        
        <div class="panel">
            <h2>Speed Control</h2>
            <div class="slider-value" id="speedValue">1000 steps/sec</div>
            <input type="range" id="speedSlider" min="0" max="5000" value="1000" 
                   oninput="updateSpeed(this.value)">
            
            <h2>Acceleration Control</h2>
            <div class="slider-value" id="accelValue">500 steps/sec²</div>
            <input type="range" id="accelSlider" min="100" max="10000" value="500" 
                   oninput="updateAcceleration(this.value)">
        </div>
    </div>
    
    <script>
        let ws = null;
        let wsReconnectTimer = null;
        
        // WebSocket connection
        function connectWebSocket() {
            const wsUrl = `ws://${window.location.host}/ws`;
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                console.log('WebSocket connected');
                updateConnectionStatus(true);
                // Subscribe to updates
                ws.send(JSON.stringify({
                    type: 'subscribe',
                    topics: ['status', 'config']
                }));
            };
            
            ws.onclose = function() {
                console.log('WebSocket disconnected');
                updateConnectionStatus(false);
                // Reconnect after 2 seconds
                if (wsReconnectTimer) clearTimeout(wsReconnectTimer);
                wsReconnectTimer = setTimeout(connectWebSocket, 2000);
            };
            
            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    handleWebSocketMessage(data);
                } catch (e) {
                    console.error('Failed to parse WebSocket message:', e);
                }
            };
        }
        
        function handleWebSocketMessage(data) {
            switch(data.type) {
                case 'status':
                    updateStatus(data.data);
                    break;
                case 'config':
                    updateConfig(data.data);
                    break;
                case 'success':
                    console.log('Success:', data.message);
                    break;
                case 'error':
                    console.error('Error:', data.message);
                    alert('Error: ' + data.message);
                    break;
            }
        }
        
        function updateStatus(status) {
            // Position
            document.getElementById('currentPos').textContent = status.position.current + ' steps';
            document.getElementById('targetPos').textContent = status.position.target + ' steps';
            document.getElementById('currentSpeed').textContent = status.speed.current.toFixed(1) + ' steps/sec';
            
            // System state
            const states = ['INIT', 'IDLE', 'READY', 'MOVING', 'HOMING', 'ERROR', 'EMERGENCY'];
            document.getElementById('systemState').textContent = states[status.state] || 'UNKNOWN';
            
            // Motor state
            document.getElementById('motorState').textContent = status.enabled ? 'ENABLED' : 'DISABLED';
            document.getElementById('enableBtn').disabled = status.enabled;
            document.getElementById('disableBtn').disabled = !status.enabled;
            
            // Limit switches
            document.getElementById('leftLimit').className = 'limit-indicator ' + 
                (status.limits.left ? 'active' : 'inactive');
            document.getElementById('rightLimit').className = 'limit-indicator ' + 
                (status.limits.right ? 'active' : 'inactive');
        }
        
        function updateConfig(config) {
            document.getElementById('maxSpeed').textContent = config.maxSpeed + ' steps/sec';
            document.getElementById('acceleration').textContent = config.acceleration + ' steps/sec²';
            document.getElementById('dmxChannel').textContent = config.dmxChannel;
            
            // Update sliders
            document.getElementById('speedSlider').value = config.maxSpeed;
            document.getElementById('speedValue').textContent = config.maxSpeed + ' steps/sec';
            document.getElementById('accelSlider').value = config.acceleration;
            document.getElementById('accelValue').textContent = config.acceleration + ' steps/sec²';
        }
        
        function updateConnectionStatus(connected) {
            const status = document.getElementById('connectionStatus');
            const text = document.getElementById('connectionText');
            
            if (connected) {
                status.className = 'connection-status';
                text.textContent = 'Connected';
            } else {
                status.className = 'connection-status disconnected';
                text.textContent = 'Disconnected';
            }
        }
        
        // Control functions
        function sendCommand(command) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    type: 'command',
                    command: command
                }));
            }
        }
        
        function moveToPosition() {
            const position = parseInt(document.getElementById('targetPosition').value);
            if (!isNaN(position)) {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({
                        type: 'command',
                        command: 'move',
                        position: position
                    }));
                }
            }
        }
        
        function jog(steps) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    type: 'command',
                    command: 'jog',
                    steps: steps
                }));
            }
        }
        
        function updateSpeed(value) {
            document.getElementById('speedValue').textContent = value + ' steps/sec';
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    type: 'config',
                    action: 'set',
                    params: {
                        maxSpeed: parseFloat(value)
                    }
                }));
            }
        }
        
        function updateAcceleration(value) {
            document.getElementById('accelValue').textContent = value + ' steps/sec²';
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({
                    type: 'config',
                    action: 'set',
                    params: {
                        acceleration: parseFloat(value)
                    }
                }));
            }
        }
        
        // Initialize on load
        window.addEventListener('load', function() {
            connectWebSocket();
            
            // Load initial status via HTTP
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    updateStatus(data);
                })
                .catch(error => console.error('Failed to load status:', error));
            
            // Load initial config
            fetch('/config')
                .then(response => response.json())
                .then(data => {
                    updateConfig(data);
                })
                .catch(error => console.error('Failed to load config:', error));
        });
    </script>
</body>
</html>
)rawliteral";

// Singleton instance
WebInterface& WebInterface::getInstance() {
    static WebInterface instance;
    return instance;
}

// Constructor
WebInterface::WebInterface() 
    : server(nullptr)
    , ws(nullptr)
    , enabled(true)
    , running(false)
    , activeClients(0)
    , nextCommandId(1)
    , apSSID(DEFAULT_AP_SSID)
    , apPassword(DEFAULT_AP_PASSWORD)
    , apChannel(DEFAULT_AP_CHANNEL)
    , apIP(192, 168, 4, 1)
    , apGateway(192, 168, 4, 1)
    , apSubnet(255, 255, 255, 0)
    , webTaskHandle(nullptr)
    , broadcastTaskHandle(nullptr) {
}

// Destructor
WebInterface::~WebInterface() {
    stop();
}

// Initialize the web interface
void WebInterface::begin() {
    if (!enabled || running) {
        return;
    }
    
    Serial.println("[WebInterface] Starting...");
    
    // Create server and websocket
    server = new AsyncWebServer(WEB_SERVER_PORT);
    ws = new AsyncWebSocket("/ws");
    
    // Setup WiFi AP
    setupWiFi();
    
    // Setup web server routes and WebSocket
    setupWebServer();
    
    // Create Core 1 tasks
    xTaskCreatePinnedToCore(
        webServerTask,
        "WebServer",
        WEB_SERVER_TASK_STACK_SIZE,
        this,
        WEB_SERVER_TASK_PRIORITY,
        &webTaskHandle,
        1  // Core 1
    );
    
    xTaskCreatePinnedToCore(
        statusBroadcastTask,
        "WebBroadcast",
        BROADCAST_TASK_STACK_SIZE,
        this,
        BROADCAST_TASK_PRIORITY,
        &broadcastTaskHandle,
        1  // Core 1
    );
    
    running = true;
    Serial.println("[WebInterface] Started successfully");
    Serial.print("[WebInterface] Access Point IP: ");
    Serial.println(WiFi.softAPIP());
}

// Update function (called from main loop if needed)
void WebInterface::update() {
    // Most work is done in tasks, but we can do cleanup here if needed
    if (ws && running) {
        ws->cleanupClients();
    }
}

// Stop the web interface
void WebInterface::stop() {
    if (!running) {
        return;
    }
    
    Serial.println("[WebInterface] Stopping...");
    
    running = false;
    
    // Stop tasks
    if (webTaskHandle) {
        vTaskDelete(webTaskHandle);
        webTaskHandle = nullptr;
    }
    
    if (broadcastTaskHandle) {
        vTaskDelete(broadcastTaskHandle);
        broadcastTaskHandle = nullptr;
    }
    
    // Stop server
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }
    
    if (ws) {
        delete ws;
        ws = nullptr;
    }
    
    // Stop WiFi
    WiFi.softAPdisconnect(true);
    
    Serial.println("[WebInterface] Stopped");
}

// Get AP IP address as string
String WebInterface::getAPAddress() const {
    return WiFi.softAPIP().toString();
}

// Set WiFi credentials
void WebInterface::setCredentials(const char* ssid, const char* password) {
    apSSID = ssid;
    apPassword = password;
}

// Enable/disable the interface
void WebInterface::setEnabled(bool enable) {
    if (enabled == enable) {
        return;
    }
    
    enabled = enable;
    
    if (enabled && !running) {
        begin();
    } else if (!enabled && running) {
        stop();
    }
}

// Setup WiFi Access Point
void WebInterface::setupWiFi() {
    Serial.println("[WebInterface] Setting up WiFi Access Point...");
    
    WiFi.mode(WIFI_AP);
    
    // Configure AP
    if (apPassword.length() > 0) {
        WiFi.softAP(apSSID.c_str(), apPassword.c_str(), apChannel, 0, AP_MAX_CONNECTIONS);
    } else {
        WiFi.softAP(apSSID.c_str(), nullptr, apChannel, 0, AP_MAX_CONNECTIONS);
    }
    
    // Configure IP
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    
    Serial.print("[WebInterface] AP SSID: ");
    Serial.println(apSSID);
    Serial.print("[WebInterface] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

// Setup web server routes
void WebInterface::setupWebServer() {
    // Attach WebSocket handler
    ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, 
                       AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    
    server->addHandler(ws);
    
    // Setup routes
    setupRoutes();
    
    // Start server
    server->begin();
    
    Serial.println("[WebInterface] Web server started on port 80");
}

// Setup HTTP routes
void WebInterface::setupRoutes() {
    // Main page
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleRoot(request);
    });
    
    // API endpoints
    server->on("/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleStatus(request);
    });
    
    server->on("/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleConfig(request);
    });
    
    server->on("/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleConfigUpdate(request);
    });
    
    server->on("/command", HTTP_POST, [this](AsyncWebServerRequest* request) {
        this->handleCommand(request);
    });
    
    server->on("/info", HTTP_GET, [this](AsyncWebServerRequest* request) {
        this->handleInfo(request);
    });
    
    // 404 handler
    server->onNotFound([this](AsyncWebServerRequest* request) {
        this->handleNotFound(request);
    });
}

// HTTP Handlers

void WebInterface::handleRoot(AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
}

void WebInterface::handleStatus(AsyncWebServerRequest* request) {
    StaticJsonDocument<512> doc;
    getSystemStatus(doc);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebInterface::handleConfig(AsyncWebServerRequest* request) {
    StaticJsonDocument<1024> doc;
    getSystemConfig(doc);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebInterface::handleCommand(AsyncWebServerRequest* request) {
    if (!request->hasParam("plain", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing request body\"}");
        return;
    }
    
    String body = request->getParam("plain", true)->value();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("command")) {
        request->send(400, "application/json", "{\"error\":\"Missing command field\"}");
        return;
    }
    
    // Validate and execute command
    if (!validateCommand(doc)) {
        request->send(400, "application/json", "{\"error\":\"Invalid command\"}");
        return;
    }
    
    String command = doc["command"];
    bool success = false;
    
    // Process motion commands
    if (command == "move") {
        int32_t position = doc["position"];
        success = sendMotionCommand(CommandType::MOVE_ABSOLUTE, position);
    } else if (command == "jog") {
        int32_t steps = doc["steps"];
        success = sendMotionCommand(CommandType::MOVE_RELATIVE, steps);
    } else if (command == "home") {
        success = sendMotionCommand(CommandType::HOME);
    } else if (command == "stop") {
        success = sendMotionCommand(CommandType::STOP);
    } else if (command == "estop") {
        success = sendMotionCommand(CommandType::EMERGENCY_STOP);
    } else if (command == "enable") {
        success = sendMotionCommand(CommandType::ENABLE);
    } else if (command == "disable") {
        success = sendMotionCommand(CommandType::DISABLE);
    }
    
    if (success) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(503, "application/json", "{\"error\":\"Command queue full\"}");
    }
}

void WebInterface::handleConfigUpdate(AsyncWebServerRequest* request) {
    if (!request->hasParam("plain", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing request body\"}");
        return;
    }
    
    String body = request->getParam("plain", true)->value();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (updateConfiguration(doc)) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(400, "application/json", "{\"error\":\"Configuration update failed\"}");
    }
}

void WebInterface::handleInfo(AsyncWebServerRequest* request) {
    StaticJsonDocument<512> doc;
    getSystemInfo(doc);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebInterface::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
}

// WebSocket event handler
void WebInterface::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                            AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch(type) {
        case WS_EVT_CONNECT: {
            Serial.printf("[WebInterface] WebSocket client #%u connected from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            
            if (activeClients >= WS_MAX_CLIENTS) {
                client->close(1013, "Try again later");
                return;
            }
            
            activeClients++;
            
            // Send initial status
            sendStatusToClient(client);
            break;
        }
        
        case WS_EVT_DISCONNECT: {
            Serial.printf("[WebInterface] WebSocket client #%u disconnected\n", client->id());
            if (activeClients > 0) {
                activeClients--;
            }
            break;
        }
        
        case WS_EVT_DATA: {
            handleWsMessage(client, data, len);
            break;
        }
        
        case WS_EVT_ERROR: {
            Serial.printf("[WebInterface] WebSocket error from client #%u: %s\n", 
                         client->id(), (char*)data);
            break;
        }
        
        case WS_EVT_PONG:
            break;
    }
}

// Handle WebSocket messages
void WebInterface::handleWsMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len) {
    // Ensure null termination
    data[len] = 0;
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, (char*)data);
    
    if (error) {
        client->text("{\"type\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }
    
    String type = doc["type"] | "";
    
    if (type == "command") {
        // Process command
        if (!validateCommand(doc)) {
            client->text("{\"type\":\"error\",\"message\":\"Invalid command\"}");
            return;
        }
        
        String command = doc["command"];
        bool success = false;
        
        if (command == "move") {
            int32_t position = doc["position"];
            success = sendMotionCommand(CommandType::MOVE_ABSOLUTE, position);
        } else if (command == "jog") {
            int32_t steps = doc["steps"];
            success = sendMotionCommand(CommandType::MOVE_RELATIVE, steps);
        } else if (command == "home") {
            success = sendMotionCommand(CommandType::HOME);
        } else if (command == "stop") {
            success = sendMotionCommand(CommandType::STOP);
        } else if (command == "estop") {
            success = sendMotionCommand(CommandType::EMERGENCY_STOP);
        } else if (command == "enable") {
            success = sendMotionCommand(CommandType::ENABLE);
        } else if (command == "disable") {
            success = sendMotionCommand(CommandType::DISABLE);
        }
        
        if (success) {
            client->text("{\"type\":\"success\",\"message\":\"Command sent\"}");
        } else {
            client->text("{\"type\":\"error\",\"message\":\"Command queue full\"}");
        }
        
    } else if (type == "config") {
        String action = doc["action"] | "";
        
        if (action == "set" && doc.containsKey("params")) {
            JsonObject params = doc["params"];
            if (updateConfiguration(params)) {
                client->text("{\"type\":\"success\",\"message\":\"Configuration updated\"}");
                // Broadcast new config to all clients
                broadcastStatus();
            } else {
                client->text("{\"type\":\"error\",\"message\":\"Configuration update failed\"}");
            }
        }
        
    } else if (type == "subscribe") {
        // Client subscribing to updates (we send to all clients anyway)
        client->text("{\"type\":\"success\",\"message\":\"Subscribed\"}");
    }
}

// Get system status
void WebInterface::getSystemStatus(JsonDocument& doc) {
    // Use thread-safe macros to read status
    int32_t currentPos, targetPos;
    float currentSpeed, maxSpeed, acceleration;
    bool stepperEnabled, leftLimit, rightLimit;
    SystemState state;
    
    SAFE_READ_STATUS(currentPosition, currentPos);
    SAFE_READ_STATUS(targetPosition, targetPos);
    SAFE_READ_STATUS(currentSpeed, currentSpeed);
    SAFE_READ_STATUS(stepperEnabled, stepperEnabled);
    SAFE_READ_STATUS(systemState, state);
    leftLimit = false;
    rightLimit = false;
    bool limitsActive[2];
    SAFE_READ_STATUS(limitsActive[0], limitsActive[0]);
    SAFE_READ_STATUS(limitsActive[1], limitsActive[1]);
    leftLimit = limitsActive[0];
    rightLimit = limitsActive[1];
    
    // Read config
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
        maxSpeed = config->defaultProfile.maxSpeed;
        acceleration = config->defaultProfile.acceleration;
    }
    
    // Build response
    JsonObject position = doc.createNestedObject("position");
    position["current"] = currentPos;
    position["target"] = targetPos;
    
    JsonObject speed = doc.createNestedObject("speed");
    speed["current"] = currentSpeed;
    speed["max"] = maxSpeed;
    
    doc["acceleration"] = acceleration;
    doc["state"] = static_cast<int>(state);
    doc["enabled"] = stepperEnabled;
    
    JsonObject limits = doc.createNestedObject("limits");
    limits["left"] = leftLimit;
    limits["right"] = rightLimit;
}

// Get system configuration
void WebInterface::getSystemConfig(JsonDocument& doc) {
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
        return;
    }
    
    doc["maxSpeed"] = config->defaultProfile.maxSpeed;
    doc["acceleration"] = config->defaultProfile.acceleration;
    doc["deceleration"] = config->defaultProfile.deceleration;
    doc["homingSpeed"] = config->homingSpeed;
    
    doc["dmxChannel"] = config->dmxStartChannel;
    doc["dmxScale"] = config->dmxScale;
    doc["dmxOffset"] = config->dmxOffset;
    
    doc["minPosition"] = config->minPosition;
    doc["maxPosition"] = config->maxPosition;
}

// Get system info
void WebInterface::getSystemInfo(JsonDocument& doc) {
    doc["version"] = "4.0.0";
    doc["module"] = "SkullStepperV4";
    doc["uptime"] = millis();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["clients"] = activeClients;
    doc["ip"] = WiFi.softAPIP().toString();
    doc["ssid"] = apSSID;
}

// Send motion command to queue
bool WebInterface::sendMotionCommand(CommandType type, int32_t position) {
    MotionCommand cmd;
    cmd.type = type;
    cmd.timestamp = millis();
    cmd.commandId = nextCommandId++;
    
    if (type == CommandType::MOVE_ABSOLUTE) {
        cmd.profile.targetPosition = position;
        // Position already set in targetPosition
    } else if (type == CommandType::MOVE_RELATIVE) {
        cmd.profile.targetPosition = position;  // Steps to move
        // Steps to move stored in targetPosition
    }
    
    // Non-blocking send
    if (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE) {
        Serial.printf("[WebInterface] Motion command queued: type=%d\n", type);
        return true;
    }
    
    Serial.println("[WebInterface] Motion command queue full!");
    return false;
}

// Update configuration
bool WebInterface::updateConfiguration(const JsonDocument& params) {
    bool success = true;
    
    // Motion parameters
    if (params.containsKey("maxSpeed")) {
        float speed = params["maxSpeed"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->defaultProfile.maxSpeed = speed;
            success = true;
        } else {
            success = false;
        }
    }
    
    if (params.containsKey("acceleration")) {
        float accel = params["acceleration"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->defaultProfile.acceleration = accel;
            success = true;
        } else {
            success = false;
        }
    }
    
    if (params.containsKey("deceleration")) {
        float decel = params["deceleration"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->defaultProfile.deceleration = decel;
            success = true;
        } else {
            success = false;
        }
    }
    
    if (params.containsKey("homingSpeed")) {
        float speed = params["homingSpeed"];
        // Homing speed is part of main config, not motion profile
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->homingSpeed = speed;
            success = true;
        } else {
            success = false;
        }
    }
    
    // DMX parameters
    if (params.containsKey("dmxChannel")) {
        uint16_t channel = params["dmxChannel"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->dmxStartChannel = channel;
            success = true;
        } else {
            success = false;
        }
    }
    
    if (params.containsKey("dmxScale")) {
        float scale = params["dmxScale"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->dmxScale = scale;
            success = true;
        } else {
            success = false;
        }
    }
    
    if (params.containsKey("dmxOffset")) {
        int32_t offset = params["dmxOffset"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (config) {
            config->dmxOffset = offset;
            success = true;
        } else {
            success = false;
        }
    }
    
    // Commit changes if successful
    if (success) {
        SystemConfigMgr::saveToEEPROM();
        Serial.println("[WebInterface] Configuration updated");
    }
    
    return success;
}

// Broadcast status to all connected clients
void WebInterface::broadcastStatus() {
    if (!ws || activeClients == 0) {
        return;
    }
    
    StaticJsonDocument<512> statusDoc;
    getSystemStatus(statusDoc);
    
    StaticJsonDocument<512> messageDoc;
    messageDoc["type"] = "status";
    messageDoc["data"] = statusDoc;
    
    String statusJson;
    serializeJson(messageDoc, statusJson);
    ws->textAll(statusJson);
}

// Send status to specific client
void WebInterface::sendStatusToClient(AsyncWebSocketClient* client) {
    if (!client || !client->canSend()) {
        return;
    }
    
    // Send status
    StaticJsonDocument<512> statusDoc;
    getSystemStatus(statusDoc);
    
    StaticJsonDocument<512> statusMessage;
    statusMessage["type"] = "status";
    statusMessage["data"] = statusDoc;
    
    String statusJson;
    serializeJson(statusMessage, statusJson);
    client->text(statusJson);
    
    // Send config
    StaticJsonDocument<512> configDoc;
    getSystemConfig(configDoc);
    
    StaticJsonDocument<512> configMessage;
    configMessage["type"] = "config";
    configMessage["data"] = configDoc;
    
    String configJson;
    serializeJson(configMessage, configJson);
    client->text(configJson);
}

// Validate command
bool WebInterface::validateCommand(const JsonDocument& cmd) {
    if (!cmd.containsKey("command")) {
        return false;
    }
    
    String command = cmd["command"];
    
    // Check system state
    SystemState state;
    SAFE_READ_STATUS(systemState, state);
    
    // In emergency state, only allow emergency stop clearing
    if (state == SystemState::EMERGENCY_STOP) {
        return (command == "clear_emergency" || command == "estop");
    }
    
    // Validate move commands
    if (command == "move") {
        if (!cmd.containsKey("position")) {
            return false;
        }
        
        int32_t pos = cmd["position"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        if (!config) {
            return false;
        }
        
        // Check position limits
        return (pos >= config->minPosition &&
                pos <= config->maxPosition);
    }
    
    // Validate jog commands
    if (command == "jog") {
        if (!cmd.containsKey("steps")) {
            return false;
        }
        
        // Could add additional validation here
        return true;
    }
    
    // Other commands are generally valid
    return true;
}

// Task functions

void WebInterface::webServerTask(void* parameter) {
    WebInterface* self = static_cast<WebInterface*>(parameter);
    
    Serial.println("[WebInterface] Web server task started on Core 1");
    
    while (self->running) {
        // Server runs asynchronously, just yield
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    vTaskDelete(NULL);
}

void WebInterface::statusBroadcastTask(void* parameter) {
    WebInterface* self = static_cast<WebInterface*>(parameter);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(STATUS_BROADCAST_INTERVAL_MS);
    
    Serial.println("[WebInterface] Broadcast task started on Core 1");
    
    while (self->running) {
        // Only broadcast if clients are connected
        if (self->activeClients > 0) {
            self->broadcastStatus();
        }
        
        // Maintain precise timing
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    
    vTaskDelete(NULL);
}

#endif // ENABLE_WEB_INTERFACE