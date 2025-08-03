/*
 * WebInterface.cpp
 * 
 * Implementation using built-in ESP32 WebServer and WebSocketsServer
 * All content served from memory - no filesystem dependencies
 */

#include "WebInterface.h"
#include "StepperController.h"  // Ensure StepperController interface is included
#include "SerialInterface.h"    // For processCommand function
#include <esp_random.h>         // For esp_random() function

#ifdef ENABLE_WEB_INTERFACE

// ============================================================================
// HTML/CSS/JS Content Generation (all dynamic)
// ============================================================================

String WebInterface::getIndexHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SkullStepper Control</title>
    <style>)rawliteral";
    
    html += getMainCSS();
    html += R"rawliteral(</style>
</head>
<body>
    <div class="container">
        <header>
            <h1>SkullStepper Control</h1>
            <div class="connection-status" id="connectionStatus">
                <span class="status-dot" id="statusDot"></span>
                <span id="statusText">Connecting...</span>
            </div>
        </header>
        
        <div class="panel">
            <h2>System Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <label>State:</label>
                    <span id="systemState" class="value">--</span>
                </div>
                <div class="status-item">
                    <label>Position:</label>
                    <span id="currentPosition" class="value">--</span>
                </div>
                <div class="status-item">
                    <label>Target:</label>
                    <span id="targetPosition" class="value">--</span>
                </div>
                <div class="status-item">
                    <label>Speed:</label>
                    <span id="currentSpeed" class="value">--</span>
                </div>
                <div class="status-item">
                    <label>Motor:</label>
                    <span id="motorEnabled" class="value">--</span>
                </div>
                <div class="status-item">
                    <label>Limits:</label>
                    <span id="limitStatus" class="value">--</span>
                </div>
            </div>
        </div>
        
        <div class="panel">
            <h2>Motion Control</h2>
            <div class="button-grid">
                <button class="btn btn-primary" onclick="sendCommand('home')">HOME</button>
                <button class="btn btn-warning" onclick="sendCommand('stop')">STOP</button>
                <button class="btn btn-danger" onclick="sendCommand('estop')">E-STOP</button>
            </div>
            
            <div class="motor-control">
                <button id="enableBtn" class="btn btn-success" onclick="toggleMotor()">ENABLE</button>
                <button id="disableBtn" class="btn btn-secondary" onclick="toggleMotor()" style="display:none;">DISABLE</button>
            </div>
            
            <div class="position-control">
                <h3>Move to Position</h3>
                <div class="input-group">
                    <input type="number" id="positionInput" placeholder="Target position" step="10">
                    <button class="btn btn-primary" onclick="moveToPosition()">MOVE</button>
                </div>
            </div>
            
            <div class="jog-control">
                <h3>Jog Control</h3>
                <div class="jog-buttons">
                    <button class="btn btn-jog" onclick="jog(-1000)">-1000</button>
                    <button class="btn btn-jog" onclick="jog(-100)">-100</button>
                    <button class="btn btn-jog" onclick="jog(-10)">-10</button>
                    <button class="btn btn-jog" onclick="jog(10)">+10</button>
                    <button class="btn btn-jog" onclick="jog(100)">+100</button>
                    <button class="btn btn-jog" onclick="jog(1000)">+1000</button>
                </div>
            </div>
            
            <div class="test-control">
                <h3>Testing</h3>
                <div class="test-buttons">
                    <button class="btn btn-test" onclick="sendCommand('test')" title="Run continuous stress test (10% to 90% of range)">STRESS TEST</button>
                    <button class="btn btn-test" onclick="sendCommand('test2')" title="Move to 10 random positions within safe range">RANDOM MOVES</button>
                </div>
                <p class="test-info">Tests require homing first. Use STOP or E-STOP buttons to stop tests.</p>
            </div>
        </div>
        
        <div class="panel">
            <h2>System Information</h2>
            <div class="info-grid">
                <div class="info-item">
                    <label>Version:</label>
                    <span id="systemVersion" class="value">--</span>
                </div>
                <div class="info-item">
                    <label>Hardware:</label>
                    <span id="systemHardware" class="value">--</span>
                </div>
                <div class="info-item">
                    <label>Uptime:</label>
                    <span id="systemUptime" class="value">--</span>
                </div>
                <div class="info-item">
                    <label>Free Memory:</label>
                    <span id="systemMemory" class="value">--</span>
                </div>
                <div class="info-item">
                    <label>Task Stack:</label>
                    <span id="taskStack" class="value">--</span>
                </div>
                <div class="info-item">
                    <label>WiFi Clients:</label>
                    <span id="wifiClients" class="value">--</span>
                </div>
            </div>
        </div>
        
        <div class="panel">
            <h2>Configuration</h2>
            
            <!-- Tab buttons -->
            <div class="config-tabs">
                <button class="tab-btn active" onclick="showConfigTab('motion')">Motion</button>
                <button class="tab-btn" onclick="showConfigTab('dmx')">DMX</button>
                <button class="tab-btn" onclick="showConfigTab('limits')">Limits</button>
                <button class="tab-btn" onclick="showConfigTab('advanced')">Advanced</button>
            </div>
            
            <!-- Motion Configuration Tab -->
            <div id="motion-tab" class="config-tab active">
                <h3>Motion Parameters</h3>
                <div class="config-item">
                    <label for="maxSpeed">Max Speed:</label>
                    <input type="range" id="maxSpeed" min="100" max="10000" step="100">
                    <span id="maxSpeedValue">--</span> steps/sec
                </div>
                <div class="config-item">
                    <label for="acceleration">Acceleration:</label>
                    <input type="range" id="acceleration" min="100" max="20000" step="100">
                    <span id="accelerationValue">--</span> steps/sec²
                </div>
                <div class="config-item">
                    <label for="homingSpeed">Homing Speed:</label>
                    <input type="range" id="homingSpeed" min="100" max="10000" step="100">
                    <span id="homingSpeedValue">--</span> steps/sec
                </div>
            </div>
            
            <!-- DMX Configuration Tab -->
            <div id="dmx-tab" class="config-tab">
                <h3>DMX Settings</h3>
                <div class="config-item">
                    <label for="dmxChannel">DMX Start Channel:</label>
                    <input type="number" id="dmxChannel" min="1" max="512" step="1">
                </div>
                <div class="config-item">
                    <label for="dmxScale">DMX Scale Factor:</label>
                    <input type="number" id="dmxScale" step="0.1" placeholder="Steps per DMX unit">
                </div>
                <div class="config-item">
                    <label for="dmxOffset">DMX Offset:</label>
                    <input type="number" id="dmxOffset" step="1" placeholder="Position offset in steps">
                </div>
            </div>
            
            <!-- Position Limits Tab -->
            <div id="limits-tab" class="config-tab">
                <h3>Position Limits</h3>
                <div class="config-item">
                    <label for="minPosition">Minimum Position:</label>
                    <input type="number" id="minPosition" step="10" placeholder="Steps">
                </div>
                <div class="config-item">
                    <label for="maxPosition">Maximum Position:</label>
                    <input type="number" id="maxPosition" step="10" placeholder="Steps">
                </div>
                <div class="config-item">
                    <label for="homePosition">Home Position:</label>
                    <input type="number" id="homePosition" step="1" placeholder="Steps">
                </div>
            </div>
            
            <!-- Advanced Configuration Tab -->
            <div id="advanced-tab" class="config-tab">
                <h3>Advanced Parameters</h3>
                <div class="config-item">
                    <label for="jerk">Jerk Limitation:</label>
                    <input type="range" id="jerk" min="0" max="50000" step="1000">
                    <span id="jerkValue">--</span> steps/sec³
                    <small class="param-info">Controls smoothness of acceleration changes (0-50000)</small>
                </div>
                <div class="config-item">
                    <label for="emergencyDeceleration">Emergency Deceleration:</label>
                    <input type="range" id="emergencyDeceleration" min="100" max="50000" step="100">
                    <span id="emergencyDecelerationValue">--</span> steps/sec²
                    <small class="param-info">Deceleration rate for emergency stops (100-50000)</small>
                </div>
                <div class="config-item">
                    <label for="dmxTimeout">DMX Timeout:</label>
                    <input type="number" id="dmxTimeout" min="100" max="60000" step="100" placeholder="Milliseconds">
                    <small class="param-info">Time before DMX signal loss is detected (100-60000 ms)</small>
                </div>
            </div>
            
            <button class="btn btn-primary" onclick="applyConfig()">Apply Changes</button>
            <button class="btn btn-secondary" onclick="loadConfig()">Reload</button>
        </div>
        
        <footer>
            <p>SkullStepperV4 - ESP32-S3 Motion Control System</p>
        </footer>
    </div>
    
    <script>)rawliteral";
    
    html += getMainJS();
    html += R"rawliteral(</script>
</body>
</html>
)rawliteral";
    
    return html;
}

String WebInterface::getMainCSS() {
    return R"rawliteral(
:root {
    --primary-color: #00d4ff;
    --success-color: #00ff88;
    --warning-color: #ffaa00;
    --danger-color: #ff0044;
    --bg-color: #0a0a0a;
    --panel-bg: #1a1a1a;
    --text-color: #ffffff;
    --text-dim: #888888;
    --border-color: #333333;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: var(--bg-color);
    color: var(--text-color);
    line-height: 1.6;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    padding: 20px;
}

header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 30px;
    padding-bottom: 20px;
    border-bottom: 1px solid var(--border-color);
}

h1 {
    color: var(--primary-color);
    font-size: 2em;
}

.connection-status {
    display: flex;
    align-items: center;
    gap: 10px;
}

.status-dot {
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: var(--danger-color);
    transition: background 0.3s;
}

.status-dot.connected {
    background: var(--success-color);
}

.panel {
    background: var(--panel-bg);
    border-radius: 10px;
    padding: 20px;
    margin-bottom: 20px;
    border: 1px solid var(--border-color);
}

.panel h2 {
    color: var(--primary-color);
    margin-bottom: 15px;
    font-size: 1.3em;
}

.panel h3 {
    color: var(--text-dim);
    margin-bottom: 10px;
    font-size: 1.1em;
}

.status-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
    gap: 15px;
}

.status-item {
    display: flex;
    justify-content: space-between;
    padding: 8px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 5px;
}

.status-item label {
    color: var(--text-dim);
}

.status-item .value {
    color: var(--primary-color);
    font-weight: bold;
}

.info-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 15px;
}

.info-item {
    display: flex;
    justify-content: space-between;
    padding: 8px;
    background: rgba(0, 0, 0, 0.3);
    border-radius: 5px;
}

.info-item label {
    color: var(--text-dim);
}

.info-item .value {
    color: var(--primary-color);
    font-weight: bold;
}

.button-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
    margin-bottom: 20px;
}

.btn {
    padding: 12px 24px;
    border: none;
    border-radius: 5px;
    font-size: 16px;
    font-weight: bold;
    cursor: pointer;
    transition: all 0.3s;
    text-transform: uppercase;
}

.btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 8px rgba(0, 0, 0, 0.3);
}

.btn:active {
    transform: translateY(0);
}

.btn-primary {
    background: var(--primary-color);
    color: var(--bg-color);
}

.btn-success {
    background: var(--success-color);
    color: var(--bg-color);
}

.btn-warning {
    background: var(--warning-color);
    color: var(--bg-color);
}

.btn-danger {
    background: var(--danger-color);
    color: white;
}

.btn-secondary {
    background: var(--text-dim);
    color: white;
}

.btn-jog {
    background: var(--panel-bg);
    color: var(--primary-color);
    border: 2px solid var(--primary-color);
    padding: 8px 16px;
}

.motor-control {
    text-align: center;
    margin: 20px 0;
}

.position-control {
    margin: 20px 0;
}

.input-group {
    display: flex;
    gap: 10px;
}

.input-group input {
    flex: 1;
    padding: 10px;
    background: rgba(0, 0, 0, 0.3);
    border: 1px solid var(--border-color);
    border-radius: 5px;
    color: var(--text-color);
    font-size: 16px;
}

.jog-buttons {
    display: flex;
    justify-content: center;
    gap: 5px;
    flex-wrap: wrap;
}

.test-control {
    margin-top: 20px;
    padding-top: 20px;
    border-top: 1px solid var(--border-color);
}

.test-buttons {
    display: flex;
    justify-content: center;
    gap: 10px;
    margin-bottom: 10px;
}

.btn-test {
    background: var(--warning-color);
    color: var(--bg-color);
    padding: 10px 24px;
    font-weight: bold;
}

.test-info {
    text-align: center;
    color: var(--text-dim);
    font-size: 0.85em;
    margin: 10px 0 0 0;
}

.config-item {
    margin-bottom: 15px;
}

.config-item label {
    display: block;
    margin-bottom: 5px;
    color: var(--text-dim);
}

.config-item input[type="range"] {
    width: 100%;
    margin-bottom: 5px;
}

.config-item input[type="number"] {
    width: 100%;
    padding: 8px;
    background: rgba(0, 0, 0, 0.3);
    border: 1px solid var(--border-color);
    border-radius: 5px;
    color: var(--text-color);
    font-size: 14px;
}

.param-info {
    display: block;
    margin-top: 5px;
    color: var(--text-dim);
    font-size: 0.85em;
    line-height: 1.3;
}

.config-tabs {
    display: flex;
    gap: 5px;
    margin-bottom: 20px;
    border-bottom: 2px solid var(--border-color);
}

.tab-btn {
    padding: 10px 20px;
    background: transparent;
    border: none;
    color: var(--text-dim);
    cursor: pointer;
    font-size: 14px;
    font-weight: bold;
    border-bottom: 3px solid transparent;
    transition: all 0.3s;
}

.tab-btn:hover {
    color: var(--text-color);
}

.tab-btn.active {
    color: var(--primary-color);
    border-bottom-color: var(--primary-color);
}

.config-tab {
    display: none;
    animation: fadeIn 0.3s;
}

.config-tab.active {
    display: block;
}

@keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

footer {
    text-align: center;
    padding-top: 20px;
    color: var(--text-dim);
    font-size: 0.9em;
}

@media (max-width: 600px) {
    .button-grid {
        grid-template-columns: 1fr;
    }
    
    .jog-buttons {
        justify-content: space-between;
    }
    
    .jog-buttons button {
        flex: 1 0 30%;
        margin: 2px;
    }
}
)rawliteral";
}

String WebInterface::getMainJS() {
    // Note: WebSocket port is on 81
    return R"rawliteral(
let ws = null;
let wsReconnectTimer = null;
let motorEnabled = false;
let isAdjustingSliders = false;  // Track if user is adjusting sliders

// WebSocket connection management
function connectWebSocket() {
    const wsUrl = `ws://${window.location.hostname}:81/`;
    console.log('Connecting to WebSocket:', wsUrl);
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
        console.log('WebSocket connected');
        updateConnectionStatus(true);
        clearTimeout(wsReconnectTimer);
        // Request initial status
        ws.send(JSON.stringify({ type: 'getStatus' }));
    };
    
    ws.onclose = () => {
        console.log('WebSocket disconnected');
        updateConnectionStatus(false);
        wsReconnectTimer = setTimeout(connectWebSocket, 3000);
    };
    
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
    
    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            updateUI(data);
        } catch (e) {
            console.error('Failed to parse WebSocket message:', e);
        }
    };
}

function updateConnectionStatus(connected) {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    
    if (connected) {
        dot.classList.add('connected');
        text.textContent = 'Connected';
    } else {
        dot.classList.remove('connected');
        text.textContent = 'Disconnected';
    }
}

// Helper function to format uptime
function formatUptime(milliseconds) {
    const seconds = Math.floor(milliseconds / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    if (days > 0) {
        return `${days}d ${hours % 24}h ${minutes % 60}m`;
    } else if (hours > 0) {
        return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${seconds % 60}s`;
    } else {
        return `${seconds}s`;
    }
}

// Helper function to format memory
function formatMemory(bytes) {
    if (bytes > 1024 * 1024) {
        return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
    } else if (bytes > 1024) {
        return (bytes / 1024).toFixed(1) + ' KB';
    } else {
        return bytes + ' B';
    }
}

function updateUI(data) {
    // Update status display
    if (data.systemState !== undefined) {
        const states = ['UNINITIALIZED', 'INITIALIZING', 'READY', 'RUNNING', 'STOPPING', 'STOPPED', 'ERROR', 'EMERGENCY_STOP'];
        let stateText = states[data.systemState] || 'UNKNOWN';
        
        // Add homing status to state display
        if (data.isHomed === false) {
            stateText = '⚠️ NOT HOMED';
            // Show warning message
            showHomingRequired();
        } else if (data.limitFaultActive) {
            stateText = '❌ LIMIT FAULT';
            showLimitFault();
        } else {
            hideWarnings();
        }
        
        document.getElementById('systemState').textContent = stateText;
    }
    
    if (data.position) {
        document.getElementById('currentPosition').textContent = data.position.current;
        document.getElementById('targetPosition').textContent = data.position.target;
    }
    
    if (data.speed !== undefined) {
        document.getElementById('currentSpeed').textContent = data.speed.toFixed(1);
    }
    
    if (data.stepperEnabled !== undefined) {
        motorEnabled = data.stepperEnabled;
        document.getElementById('motorEnabled').textContent = motorEnabled ? 'ENABLED' : 'DISABLED';
        updateMotorButton();
    }
    
    if (data.limits) {
        const left = data.limits.left ? 'LEFT' : '';
        const right = data.limits.right ? 'RIGHT' : '';
        const status = (left || right) ? `${left} ${right}`.trim() : 'OK';
        document.getElementById('limitStatus').textContent = status;
    }
    
    // Update system information
    if (data.systemInfo) {
        if (data.systemInfo.version) {
            document.getElementById('systemVersion').textContent = data.systemInfo.version;
        }
        if (data.systemInfo.hardware) {
            document.getElementById('systemHardware').textContent = data.systemInfo.hardware;
        }
        if (data.systemInfo.uptime !== undefined) {
            document.getElementById('systemUptime').textContent = formatUptime(data.systemInfo.uptime);
        }
        if (data.systemInfo.freeHeap !== undefined) {
            document.getElementById('systemMemory').textContent = formatMemory(data.systemInfo.freeHeap);
        }
        if (data.systemInfo.taskStackHighWaterMark !== undefined) {
            document.getElementById('taskStack').textContent = data.systemInfo.taskStackHighWaterMark + ' bytes';
        }
        if (data.systemInfo.wifiClients !== undefined) {
            document.getElementById('wifiClients').textContent = data.systemInfo.wifiClients + ' / ' + (data.systemInfo.maxClients || 2);
        }
    }
    
    // Only update config values if user is not actively adjusting them
    if (data.config && !isAdjustingSliders) {
        // Motion parameters
        if (data.config.maxSpeed !== undefined) {
            document.getElementById('maxSpeed').value = data.config.maxSpeed;
            document.getElementById('maxSpeedValue').textContent = data.config.maxSpeed;
        }
        if (data.config.acceleration !== undefined) {
            document.getElementById('acceleration').value = data.config.acceleration;
            document.getElementById('accelerationValue').textContent = data.config.acceleration;
        }
        if (data.config.homingSpeed !== undefined) {
            document.getElementById('homingSpeed').value = data.config.homingSpeed;
            document.getElementById('homingSpeedValue').textContent = data.config.homingSpeed;
        }
        if (data.config.jerk !== undefined) {
            document.getElementById('jerk').value = data.config.jerk;
            document.getElementById('jerkValue').textContent = data.config.jerk;
        }
        if (data.config.emergencyDeceleration !== undefined) {
            document.getElementById('emergencyDeceleration').value = data.config.emergencyDeceleration;
            document.getElementById('emergencyDecelerationValue').textContent = data.config.emergencyDeceleration;
        }
        
        // DMX parameters
        if (data.config.dmxChannel !== undefined) {
            document.getElementById('dmxChannel').value = data.config.dmxChannel;
        }
        if (data.config.dmxScale !== undefined) {
            document.getElementById('dmxScale').value = data.config.dmxScale;
        }
        if (data.config.dmxOffset !== undefined) {
            document.getElementById('dmxOffset').value = data.config.dmxOffset;
        }
        if (data.config.dmxTimeout !== undefined) {
            document.getElementById('dmxTimeout').value = data.config.dmxTimeout;
        }
        
        // Position limits
        if (data.config.minPosition !== undefined) {
            document.getElementById('minPosition').value = data.config.minPosition;
        }
        if (data.config.maxPosition !== undefined) {
            document.getElementById('maxPosition').value = data.config.maxPosition;
        }
        if (data.config.homePosition !== undefined) {
            document.getElementById('homePosition').value = data.config.homePosition;
        }
    }
}

function updateMotorButton() {
    const enableBtn = document.getElementById('enableBtn');
    const disableBtn = document.getElementById('disableBtn');
    
    if (motorEnabled) {
        enableBtn.style.display = 'none';
        disableBtn.style.display = 'inline-block';
    } else {
        enableBtn.style.display = 'inline-block';
        disableBtn.style.display = 'none';
    }
}

function sendCommand(command, params = {}) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const message = JSON.stringify({
            type: 'command',
            command: command,
            ...params
        });
        ws.send(message);
    } else {
        alert('Not connected to server');
    }
}

function toggleMotor() {
    sendCommand(motorEnabled ? 'disable' : 'enable');
}

function moveToPosition() {
    const input = document.getElementById('positionInput');
    const position = parseInt(input.value);
    
    if (!isNaN(position)) {
        sendCommand('move', { position: position });
        input.value = '';
    }
}

function jog(steps) {
    sendCommand('jog', { steps: steps });
}

function applyConfig() {
    // Gather all configuration values
    const config = {
        maxSpeed: parseInt(document.getElementById('maxSpeed').value),
        acceleration: parseInt(document.getElementById('acceleration').value),
        homingSpeed: parseInt(document.getElementById('homingSpeed').value),
        jerk: parseInt(document.getElementById('jerk').value),
        emergencyDeceleration: parseInt(document.getElementById('emergencyDeceleration').value),
        dmxChannel: parseInt(document.getElementById('dmxChannel').value),
        dmxScale: parseFloat(document.getElementById('dmxScale').value),
        dmxOffset: parseInt(document.getElementById('dmxOffset').value),
        dmxTimeout: parseInt(document.getElementById('dmxTimeout').value),
        minPosition: parseInt(document.getElementById('minPosition').value),
        maxPosition: parseInt(document.getElementById('maxPosition').value),
        homePosition: parseInt(document.getElementById('homePosition').value)
    };
    
    // Remove any NaN values
    Object.keys(config).forEach(key => {
        if (isNaN(config[key])) {
            delete config[key];
        }
    });
    
    console.log('Applying config:', config);
    
    sendCommand('config', {
        params: config
    });
    
    // Keep the flag set for a bit longer to allow the config to propagate
    setTimeout(() => {
        isAdjustingSliders = false;
        // Request fresh status after config update
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ type: 'getStatus' }));
        }
    }, 1000);  // Wait 1 second before allowing updates again
}

function loadConfig() {
    // Request current configuration from server
    isAdjustingSliders = false;
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'getConfig' }));
    }
}

function showConfigTab(tabName) {
    // Hide all tabs
    document.querySelectorAll('.config-tab').forEach(tab => {
        tab.classList.remove('active');
    });
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    
    // Show selected tab
    document.getElementById(tabName + '-tab').classList.add('active');
    event.target.classList.add('active');
}

// Initialize sliders with interaction tracking
document.getElementById('maxSpeed').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('maxSpeedValue').textContent = e.target.value;
});

document.getElementById('acceleration').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('accelerationValue').textContent = e.target.value;
});

document.getElementById('homingSpeed').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('homingSpeedValue').textContent = e.target.value;
});

document.getElementById('jerk').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('jerkValue').textContent = e.target.value;
});

document.getElementById('emergencyDeceleration').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('emergencyDecelerationValue').textContent = e.target.value;
});

// Track all input interactions
document.querySelectorAll('input').forEach(input => {
    input.addEventListener('focus', () => {
        isAdjustingSliders = true;
    });
    input.addEventListener('input', () => {
        isAdjustingSliders = true;
    });
});

// Also track when user starts interacting with sliders
document.getElementById('maxSpeed').addEventListener('mousedown', () => {
    isAdjustingSliders = true;
});

document.getElementById('acceleration').addEventListener('mousedown', () => {
    isAdjustingSliders = true;
});

document.getElementById('homingSpeed').addEventListener('mousedown', () => {
    isAdjustingSliders = true;
});

document.getElementById('jerk').addEventListener('mousedown', () => {
    isAdjustingSliders = true;
});

document.getElementById('emergencyDeceleration').addEventListener('mousedown', () => {
    isAdjustingSliders = true;
});

// For touch devices
document.getElementById('maxSpeed').addEventListener('touchstart', () => {
    isAdjustingSliders = true;
});

document.getElementById('acceleration').addEventListener('touchstart', () => {
    isAdjustingSliders = true;
});

document.getElementById('homingSpeed').addEventListener('touchstart', () => {
    isAdjustingSliders = true;
});

document.getElementById('jerk').addEventListener('touchstart', () => {
    isAdjustingSliders = true;
});

document.getElementById('emergencyDeceleration').addEventListener('touchstart', () => {
    isAdjustingSliders = true;
});

// Handle enter key in position input
document.getElementById('positionInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        moveToPosition();
    }
});

// Helper functions for warnings and control state
function showHomingRequired() {
    // Find or create warning div
    let warning = document.getElementById('homingWarning');
    if (!warning) {
        warning = document.createElement('div');
        warning.id = 'homingWarning';
        warning.style.cssText = 'background: #ff6600; color: white; padding: 10px; text-align: center; font-weight: bold; margin: 10px 0;';
        warning.innerHTML = '⚠️ HOMING REQUIRED - Use HOME button to establish position limits before movement';
        document.querySelector('.container').insertBefore(warning, document.querySelector('.panel'));
    }
    warning.style.display = 'block';
    
    // Disable movement controls
    disableMovementControls();
}

function showLimitFault() {
    let warning = document.getElementById('limitWarning');
    if (!warning) {
        warning = document.createElement('div');
        warning.id = 'limitWarning';
        warning.style.cssText = 'background: #ff0044; color: white; padding: 10px; text-align: center; font-weight: bold; margin: 10px 0;';
        warning.innerHTML = '❌ LIMIT FAULT - Unexpected limit switch activation. HOME required to clear fault.';
        document.querySelector('.container').insertBefore(warning, document.querySelector('.panel'));
    }
    warning.style.display = 'block';
    
    // Disable movement controls
    disableMovementControls();
}

function hideWarnings() {
    const homingWarning = document.getElementById('homingWarning');
    if (homingWarning) homingWarning.style.display = 'none';
    
    const limitWarning = document.getElementById('limitWarning');
    if (limitWarning) limitWarning.style.display = 'none';
    
    // Enable movement controls
    enableMovementControls();
}

function disableMovementControls() {
    // Disable move and jog buttons
    document.querySelectorAll('.btn-jog').forEach(btn => {
        btn.disabled = true;
        btn.style.opacity = '0.5';
    });
    
    const moveBtn = document.querySelector('button[onclick="moveToPosition()"]');
    if (moveBtn) {
        moveBtn.disabled = true;
        moveBtn.style.opacity = '0.5';
    }
    
    const posInput = document.getElementById('positionInput');
    if (posInput) {
        posInput.disabled = true;
        posInput.placeholder = 'Homing required';
    }
    
    // Disable test buttons
    document.querySelectorAll('.btn-test').forEach(btn => {
        btn.disabled = true;
        btn.style.opacity = '0.5';
    });
}

function enableMovementControls() {
    // Enable move and jog buttons
    document.querySelectorAll('.btn-jog').forEach(btn => {
        btn.disabled = false;
        btn.style.opacity = '1';
    });
    
    const moveBtn = document.querySelector('button[onclick="moveToPosition()"]');
    if (moveBtn) {
        moveBtn.disabled = false;
        moveBtn.style.opacity = '1';
    }
    
    const posInput = document.getElementById('positionInput');
    if (posInput) {
        posInput.disabled = false;
        posInput.placeholder = 'Target position';
    }
    
    // Enable test buttons
    document.querySelectorAll('.btn-test').forEach(btn => {
        btn.disabled = false;
        btn.style.opacity = '1';
    });
}

// Start WebSocket connection when page loads
window.addEventListener('load', () => {
    connectWebSocket();
});
)rawliteral";
}

// ============================================================================
// Test State Variables
// ============================================================================

// Stress test state
static bool g_stressTestActive = false;
static int32_t g_testPos1 = 0;
static int32_t g_testPos2 = 0;
static bool g_testMovingToPos2 = true;
static uint32_t g_testMoveCount = 0;
static uint32_t g_lastTestCheckTime = 0;

// Random test state  
static bool g_randomTestActive = false;
static int32_t g_randomPositions[10];
static uint8_t g_randomTestIndex = 0;
static uint32_t g_randomTestMoveCount = 0;

// ============================================================================
// Singleton Implementation
// ============================================================================

WebInterface& WebInterface::getInstance() {
    static WebInterface instance;
    return instance;
}

WebInterface::WebInterface() 
    : httpServer(nullptr)
    , wsServer(nullptr)
    , dnsServer(nullptr)
    , activeClients(0)
    , enabled(true)
    , running(false)
    , nextCommandId(1)
    , apSSID(DEFAULT_AP_SSID)
    , apPassword(DEFAULT_AP_PASSWORD)
    , apChannel(DEFAULT_AP_CHANNEL)
    , apIP(192, 168, 4, 1)
    , apGateway(192, 168, 4, 1)
    , apSubnet(255, 255, 255, 0)
    , webTaskHandle(nullptr)
    , broadcastTaskHandle(nullptr) {
    
    clientMutex = xSemaphoreCreateMutex();
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        clientConnected[i] = false;
    }
}

WebInterface::~WebInterface() {
    stop();
    if (clientMutex) {
        vSemaphoreDelete(clientMutex);
    }
}

// ============================================================================
// Module Control
// ============================================================================

void WebInterface::begin() {
    if (!enabled || running) return;
    
    Serial.println("[WebInterface] Starting Web and WebSocket servers...");
    
    // Setup WiFi AP
    setupWiFi();
    
    // Create server instances
    httpServer = new WebServer(WEB_SERVER_PORT);
    wsServer = new WebSocketsServer(WS_SERVER_PORT);
    dnsServer = new DNSServer();
    
    // Setup servers
    setupWebServer();
    setupWebSocket();
    
    // Start servers
    httpServer->begin();
    wsServer->begin();
    
    // Start DNS server for captive portal
    // This redirects all DNS requests to our IP address
    const byte DNS_PORT = 53;
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
    
    Serial.println("[WebInterface] DNS server started for captive portal");
    
    // Create tasks on Core 1
    xTaskCreatePinnedToCore(
        webServerTask,
        "WebServer",
        WEB_TASK_STACK_SIZE,
        this,
        WEB_TASK_PRIORITY,
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
    Serial.printf("[WebInterface] Servers started - HTTP: http://%s, WS: ws://%s:81\n", 
                  WiFi.softAPIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
}

void WebInterface::stop() {
    if (!running) return;
    
    Serial.println("[WebInterface] Stopping servers...");
    
    // Stop tasks
    if (webTaskHandle) {
        vTaskDelete(webTaskHandle);
        webTaskHandle = nullptr;
    }
    
    if (broadcastTaskHandle) {
        vTaskDelete(broadcastTaskHandle);
        broadcastTaskHandle = nullptr;
    }
    
    // Stop servers
    if (httpServer) {
        httpServer->stop();
        delete httpServer;
        httpServer = nullptr;
    }
    
    if (wsServer) {
        wsServer->close();
        delete wsServer;
        wsServer = nullptr;
    }
    
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    // Stop WiFi
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    
    running = false;
    activeClients = 0;
    
    Serial.println("[WebInterface] Servers stopped");
}

void WebInterface::update() {
    // Called from main loop - servers handle themselves in tasks
    
    // Update stress test if active
    updateStressTest();
    
    // Update random test if active
    updateRandomTest();
}

// ============================================================================
// Test Update Functions
// ============================================================================

void WebInterface::updateStressTest() {
    if (!g_stressTestActive) return;
    
    // Check if limit fault is active - stop test if so
    if (StepperController::isLimitFaultActive()) {
        g_stressTestActive = false;
        // Broadcast error to all WebSocket clients
        String errorMsg = "{\"status\":\"error\",\"message\":\"Stress test aborted - limit fault detected. Homing required.\"}";
        for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
            if (clientConnected[i] && wsServer) {
                wsServer->sendTXT(i, errorMsg);
            }
        }
        return;
    }
    
    // Check motion state every 100ms
    uint32_t currentTime = millis();
    if (currentTime - g_lastTestCheckTime < 100) {
        return;
    }
    g_lastTestCheckTime = currentTime;
    
    // Check if stepper is moving
    if (!StepperController::isMoving()) {
        // Movement completed, send next move
        g_testMoveCount++;
        
        // Toggle direction
        int32_t targetPos = g_testMovingToPos2 ? g_testPos1 : g_testPos2;
        g_testMovingToPos2 = !g_testMovingToPos2;
        
        // Show progress every 10 moves
        if (g_testMoveCount % 10 == 0) {
            String statusMsg = "{\"status\":\"info\",\"message\":\"Test cycle " + String(g_testMoveCount / 2) + " completed\"}";
            for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
                if (clientConnected[i] && wsServer) {
                    wsServer->sendTXT(i, statusMsg);
                }
            }
        }
        
        // Send next move
        sendMotionCommand(CommandType::MOVE_ABSOLUTE, targetPos);
    }
}

void WebInterface::updateRandomTest() {
    if (!g_randomTestActive) return;
    
    // Check if limit fault is active - stop test if so
    if (StepperController::isLimitFaultActive()) {
        g_randomTestActive = false;
        // Broadcast error to all WebSocket clients
        String errorMsg = "{\"status\":\"error\",\"message\":\"Random moves aborted - limit fault detected. Homing required.\"}";
        for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
            if (clientConnected[i] && wsServer) {
                wsServer->sendTXT(i, errorMsg);
            }
        }
        return;
    }
    
    // Check motion state every 100ms
    uint32_t currentTime = millis();
    if (currentTime - g_lastTestCheckTime < 100) {
        return;
    }
    g_lastTestCheckTime = currentTime;
    
    // Check if stepper is moving
    if (!StepperController::isMoving()) {
        // Movement completed
        g_randomTestMoveCount++;
        
        // Check if we've completed all 10 positions
        if (g_randomTestIndex >= 9) {
            // Test complete
            g_randomTestActive = false;
            String completeMsg = "{\"status\":\"info\",\"message\":\"Random moves complete - visited " + String(g_randomTestMoveCount) + " positions\"}";
            for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
                if (clientConnected[i] && wsServer) {
                    wsServer->sendTXT(i, completeMsg);
                }
            }
            return;
        }
        
        // Move to next position
        g_randomTestIndex++;
        int32_t targetPos = g_randomPositions[g_randomTestIndex];
        
        String progressMsg = "{\"status\":\"info\",\"message\":\"Moving to position " + String(g_randomTestIndex + 1) + " of 10: " + String(targetPos) + " steps\"}";
        for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
            if (clientConnected[i] && wsServer) {
                wsServer->sendTXT(i, progressMsg);
            }
        }
        
        // Send next move
        sendMotionCommand(CommandType::MOVE_ABSOLUTE, targetPos);
    }
}

// ============================================================================
// Task Functions
// ============================================================================

void WebInterface::webServerTask(void* parameter) {
    WebInterface* self = static_cast<WebInterface*>(parameter);
    
    while (true) {
        if (self->httpServer) {
            self->httpServer->handleClient();
        }
        if (self->wsServer) {
            self->wsServer->loop();
        }
        if (self->dnsServer) {
            self->dnsServer->processNextRequest();
        }
        vTaskDelay(1);  // Small delay to prevent watchdog
    }
}

void WebInterface::statusBroadcastTask(void* parameter) {
    WebInterface* self = static_cast<WebInterface*>(parameter);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(STATUS_BROADCAST_INTERVAL_MS);
    
    while (true) {
        if (self->activeClients > 0) {
            self->broadcastStatus();
        }
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// WiFi Setup
// ============================================================================

void WebInterface::setupWiFi() {
    WiFi.mode(WIFI_AP);
    
    if (apPassword.length() > 0) {
        WiFi.softAP(apSSID.c_str(), apPassword.c_str(), apChannel, 0, AP_MAX_CONNECTIONS);
    } else {
        WiFi.softAP(apSSID.c_str(), nullptr, apChannel, 0, AP_MAX_CONNECTIONS);
    }
    
    delay(100);
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    
    Serial.printf("[WebInterface] AP Started - SSID: %s, IP: %s\n", 
                  apSSID.c_str(), WiFi.softAPIP().toString().c_str());
}

// ============================================================================
// Web Server Setup
// ============================================================================

void WebInterface::setupWebServer() {
    // Main page
    httpServer->on("/", [this]() { this->handleRoot(); });
    
    // Captive portal endpoints - these URLs are used by different devices
    // to detect internet connectivity
    httpServer->on("/generate_204", [this]() { this->handleCaptivePortal(); });        // Android
    httpServer->on("/connecttest.txt", [this]() { this->handleCaptivePortal(); });     // Windows
    httpServer->on("/hotspot-detect.html", [this]() { this->handleCaptivePortal(); }); // Apple iOS/macOS
    httpServer->on("/library/test/success.html", [this]() { this->handleCaptivePortal(); }); // Apple iOS
    httpServer->on("/success.txt", [this]() { this->handleCaptivePortal(); });         // Apple iOS
    httpServer->on("/ncsi.txt", [this]() { this->handleCaptivePortal(); });            // Windows
    httpServer->on("/canonical.html", [this]() { this->handleCaptivePortal(); });      // Firefox
    
    // Common router/connectivity check URLs
    httpServer->on("/redirect", [this]() { this->handleCaptivePortal(); });
    httpServer->on("/hotspot", [this]() { this->handleCaptivePortal(); });
    httpServer->on("/favicon.ico", [this]() { this->handleFavicon(); });
    
    // API endpoints
    httpServer->on("/api/status", HTTP_GET, [this]() { this->handleStatus(); });
    httpServer->on("/api/command", HTTP_POST, [this]() { this->handleCommand(); });
    httpServer->on("/api/config", HTTP_GET, [this]() { this->handleConfig(); });
    httpServer->on("/api/config", HTTP_POST, [this]() { this->handleConfigUpdate(); });
    httpServer->on("/api/info", HTTP_GET, [this]() { this->handleInfo(); });
    
    // 404 handler - also redirect to main page for captive portal
    httpServer->onNotFound([this]() { this->handleCaptivePortal(); });
}

void WebInterface::setupWebSocket() {
    wsServer->onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        this->onWebSocketEvent(num, type, payload, length);
    });
}

// ============================================================================
// HTTP Request Handlers
// ============================================================================

void WebInterface::handleRoot() {
    httpServer->send(200, "text/html", getIndexHTML());
}

void WebInterface::handleStatus() {
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    getSystemStatus(doc);
    sendJsonResponse(200, doc);
}

void WebInterface::handleCommand() {
    if (!httpServer->hasArg("plain")) {
        sendJsonResponse(400, "error", "Missing body");
        return;
    }
    
    String body = httpServer->arg("plain");
    StaticJsonDocument<512> cmd;
    DeserializationError error = deserializeJson(cmd, body);
    
    if (error) {
        sendJsonResponse(400, "error", "Invalid JSON");
        return;
    }
    
    if (!cmd.containsKey("command")) {
        sendJsonResponse(400, "error", "Missing command field");
        return;
    }
    
    String command = cmd["command"];
    
    // Process commands
    if (command == "move") {
        if (!cmd.containsKey("position")) {
            sendJsonResponse(400, "error", "Missing position field");
            return;
        }
        int32_t position = cmd["position"];
        if (sendMotionCommand(CommandType::MOVE_ABSOLUTE, position)) {
            sendJsonResponse(200, "ok", "Move command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "jog") {
        if (!cmd.containsKey("steps")) {
            sendJsonResponse(400, "error", "Missing steps field");
            return;
        }
        int32_t steps = cmd["steps"];
        if (sendMotionCommand(CommandType::MOVE_RELATIVE, steps)) {
            sendJsonResponse(200, "ok", "Jog command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "home") {
        if (sendMotionCommand(CommandType::HOME)) {
            sendJsonResponse(200, "ok", "Home command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "stop") {
        // Stop any active tests
        if (g_stressTestActive || g_randomTestActive) {
            g_stressTestActive = false;
            g_randomTestActive = false;
        }
        if (sendMotionCommand(CommandType::STOP)) {
            sendJsonResponse(200, "ok", "Stop command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "estop") {
        // Stop any active tests
        if (g_stressTestActive || g_randomTestActive) {
            g_stressTestActive = false;
            g_randomTestActive = false;
        }
        if (sendMotionCommand(CommandType::EMERGENCY_STOP)) {
            sendJsonResponse(200, "ok", "Emergency stop command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "enable") {
        if (sendMotionCommand(CommandType::ENABLE)) {
            sendJsonResponse(200, "ok", "Enable command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "disable") {
        if (sendMotionCommand(CommandType::DISABLE)) {
            sendJsonResponse(200, "ok", "Disable command queued");
        } else {
            sendJsonResponse(503, "error", "Command queue full");
        }
    }
    else if (command == "test") {
        // Check if homed first
        if (!StepperController::isHomed()) {
            sendJsonResponse(400, "error", "System must be homed before running test");
            return;
        }
        
        // Stop any active tests
        if (g_stressTestActive || g_randomTestActive) {
            g_stressTestActive = false;
            g_randomTestActive = false;
            sendMotionCommand(CommandType::STOP);
        }
        
        // Get position limits and start test
        int32_t minPos, maxPos;
        if (StepperController::getPositionLimits(minPos, maxPos)) {
            int32_t range = maxPos - minPos;
            g_testPos1 = minPos + (range * 10 / 100);
            g_testPos2 = minPos + (range * 90 / 100);
            
            // Initialize stress test state
            g_stressTestActive = true;
            g_testMovingToPos2 = true;
            g_testMoveCount = 0;
            g_lastTestCheckTime = millis();
            
            // Send first test movement
            if (sendMotionCommand(CommandType::MOVE_ABSOLUTE, g_testPos2)) {
                sendJsonResponse(200, "ok", "Stress test started - moving between 10% and 90% of range continuously");
            } else {
                g_stressTestActive = false;
                sendJsonResponse(503, "error", "Failed to start test");
            }
        } else {
            sendJsonResponse(400, "error", "Unable to get position limits");
        }
    }
    else if (command == "test2") {
        // Check if homed first
        if (!StepperController::isHomed()) {
            sendJsonResponse(400, "error", "System must be homed before running test");
            return;
        }
        
        // Stop any active tests
        if (g_stressTestActive || g_randomTestActive) {
            g_stressTestActive = false;
            g_randomTestActive = false;
            sendMotionCommand(CommandType::STOP);
        }
        
        // Get position limits for random test
        int32_t minPos, maxPos;
        if (StepperController::getPositionLimits(minPos, maxPos)) {
            int32_t range = maxPos - minPos;
            int32_t safeMin = minPos + (range * 10 / 100);
            int32_t safeMax = minPos + (range * 90 / 100);
            int32_t safeRange = safeMax - safeMin;
            
            // Generate 10 random positions
            for (int i = 0; i < 10; i++) {
                g_randomPositions[i] = safeMin + (esp_random() % safeRange);
            }
            
            // Initialize random test state
            g_randomTestActive = true;
            g_randomTestIndex = 0;
            g_randomTestMoveCount = 0;
            g_lastTestCheckTime = millis();
            
            // Send first test movement
            if (sendMotionCommand(CommandType::MOVE_ABSOLUTE, g_randomPositions[0])) {
                sendJsonResponse(200, "ok", "Random moves started - moving to 10 random positions");
            } else {
                g_randomTestActive = false;
                sendJsonResponse(503, "error", "Failed to start test");
            }
        } else {
            sendJsonResponse(400, "error", "Unable to get position limits");
        }
    }
    else {
        sendJsonResponse(400, "error", "Unknown command");
    }
}

void WebInterface::handleConfig() {
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    getSystemConfig(doc);
    sendJsonResponse(200, doc);
}

void WebInterface::handleConfigUpdate() {
    if (!httpServer->hasArg("plain")) {
        sendJsonResponse(400, "error", "Missing body");
        return;
    }
    
    String body = httpServer->arg("plain");
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        sendJsonResponse(400, "error", "Invalid JSON");
        return;
    }
    
    if (updateConfiguration(doc)) {
        sendJsonResponse(200, "ok", "Configuration updated");
    } else {
        sendJsonResponse(400, "error", "Configuration update failed");
    }
}

void WebInterface::handleInfo() {
    StaticJsonDocument<512> doc;
    getSystemInfo(doc);
    sendJsonResponse(200, doc);
}

void WebInterface::handleNotFound() {
    sendJsonResponse(404, "error", "Not found");
}

void WebInterface::handleCaptivePortal() {
    // Send a redirect to the main page
    // This triggers the captive portal detection on most devices
    httpServer->sendHeader("Location", "http://192.168.4.1/", true);
    httpServer->send(302, "text/plain", "Redirecting to SkullStepper Control Panel");
}

void WebInterface::handleFavicon() {
    // Send empty favicon to prevent 404 errors
    httpServer->send(204, "image/x-icon", "");
}

// ============================================================================
// WebSocket Handlers
// ============================================================================

void WebInterface::onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            xSemaphoreTake(clientMutex, portMAX_DELAY);
            if (clientConnected[num]) {
                clientConnected[num] = false;
                if (activeClients > 0) activeClients--;
            }
            xSemaphoreGive(clientMutex);
            Serial.printf("[WebInterface] Client %u disconnected. Active: %d\n", num, activeClients);
            break;
            
        case WStype_CONNECTED:
            xSemaphoreTake(clientMutex, portMAX_DELAY);
            if (!clientConnected[num] && activeClients < WS_MAX_CLIENTS) {
                clientConnected[num] = true;
                activeClients++;
                xSemaphoreGive(clientMutex);
                Serial.printf("[WebInterface] Client %u connected. Active: %d\n", num, activeClients);
                // Send initial status
                sendStatusToClient(num);
            } else {
                xSemaphoreGive(clientMutex);
                wsServer->disconnect(num);
            }
            break;
            
        case WStype_TEXT:
            processWebSocketMessage(num, payload, length);
            break;
            
        case WStype_ERROR:
            Serial.printf("[WebInterface] WebSocket error from client %u\n", num);
            break;
            
        default:
            break;
    }
}

void WebInterface::processWebSocketMessage(uint8_t num, uint8_t* payload, size_t length) {
    StaticJsonDocument<512> cmd;
    DeserializationError error = deserializeJson(cmd, payload, length);
    
    if (error) {
        Serial.printf("[WebInterface] JSON parse error: %s\n", error.c_str());
        return;
    }
    
    String type = cmd["type"] | "";
    
    if (type == "command") {
        String command = cmd["command"] | "";
        
        if (command == "move") {
            int32_t position = cmd["position"];
            sendMotionCommand(CommandType::MOVE_ABSOLUTE, position);
        }
        else if (command == "jog") {
            int32_t steps = cmd["steps"];
            sendMotionCommand(CommandType::MOVE_RELATIVE, steps);
        }
        else if (command == "home") {
            sendMotionCommand(CommandType::HOME);
        }
        else if (command == "stop") {
            // Stop any active tests
            if (g_stressTestActive || g_randomTestActive) {
                g_stressTestActive = false;
                g_randomTestActive = false;
                String statusMsg = "{\"status\":\"info\",\"message\":\"Test stopped by user\"}";
                wsServer->sendTXT(num, statusMsg);
            }
            sendMotionCommand(CommandType::STOP);
        }
        else if (command == "estop") {
            // Stop any active tests
            if (g_stressTestActive || g_randomTestActive) {
                g_stressTestActive = false;
                g_randomTestActive = false;
                String statusMsg = "{\"status\":\"info\",\"message\":\"Test stopped by emergency stop\"}";
                wsServer->sendTXT(num, statusMsg);
            }
            sendMotionCommand(CommandType::EMERGENCY_STOP);
        }
        else if (command == "enable") {
            sendMotionCommand(CommandType::ENABLE);
        }
        else if (command == "disable") {
            sendMotionCommand(CommandType::DISABLE);
        }
        else if (command == "test") {
            // Check if homed first
            if (!StepperController::isHomed()) {
                // Send error response via WebSocket
                String errorMsg = "{\"status\":\"error\",\"message\":\"System must be homed before running test\"}";
                wsServer->sendTXT(num, errorMsg);
                return;
            }
            
            // Stop any active tests
            if (g_stressTestActive || g_randomTestActive) {
                g_stressTestActive = false;
                g_randomTestActive = false;
                sendMotionCommand(CommandType::STOP);
            }
            
            // Get position limits and start continuous test
            int32_t minPos, maxPos;
            if (StepperController::getPositionLimits(minPos, maxPos)) {
                int32_t range = maxPos - minPos;
                g_testPos1 = minPos + (range * 10 / 100);
                g_testPos2 = minPos + (range * 90 / 100);
                
                // Initialize stress test state
                g_stressTestActive = true;
                g_testMovingToPos2 = true;
                g_testMoveCount = 0;
                g_lastTestCheckTime = millis();
                
                // Send first test movement
                sendMotionCommand(CommandType::MOVE_ABSOLUTE, g_testPos2);
                
                // Send status message
                String statusMsg = "{\"status\":\"info\",\"message\":\"Stress test started - moving between 10% and 90% of range continuously\"}";
                wsServer->sendTXT(num, statusMsg);
            }
        }
        else if (command == "test2") {
            // Check if homed first
            if (!StepperController::isHomed()) {
                // Send error response via WebSocket
                String errorMsg = "{\"status\":\"error\",\"message\":\"System must be homed before running test\"}";
                wsServer->sendTXT(num, errorMsg);
                return;
            }
            
            // Stop any active tests
            if (g_stressTestActive || g_randomTestActive) {
                g_stressTestActive = false;
                g_randomTestActive = false;
                sendMotionCommand(CommandType::STOP);
            }
            
            // Get position limits for random test
            int32_t minPos, maxPos;
            if (StepperController::getPositionLimits(minPos, maxPos)) {
                int32_t range = maxPos - minPos;
                int32_t safeMin = minPos + (range * 10 / 100);
                int32_t safeMax = minPos + (range * 90 / 100);
                int32_t safeRange = safeMax - safeMin;
                
                // Generate 10 random positions
                for (int i = 0; i < 10; i++) {
                    g_randomPositions[i] = safeMin + (esp_random() % safeRange);
                }
                
                // Initialize random test state
                g_randomTestActive = true;
                g_randomTestIndex = 0;
                g_randomTestMoveCount = 0;
                g_lastTestCheckTime = millis();
                
                // Send first test movement
                sendMotionCommand(CommandType::MOVE_ABSOLUTE, g_randomPositions[0]);
                
                // Send status message
                String statusMsg = "{\"status\":\"info\",\"message\":\"Random moves started - moving to 10 random positions\"}";
                wsServer->sendTXT(num, statusMsg);
            }
        }
        else if (command == "config") {
            // Handle config update from WebSocket
            if (cmd.containsKey("params")) {
                if (updateConfiguration(cmd["params"])) {
                    Serial.println("[WebInterface] Configuration updated via WebSocket");
                } else {
                    Serial.println("[WebInterface] Configuration update failed");
                }
            }
        }
    }
    else if (type == "getStatus") {
        sendStatusToClient(num);
    }
}

// ============================================================================
// Internal Methods
// ============================================================================

void WebInterface::getSystemStatus(JsonDocument& doc) {
    // Read system status using thread-safe macros
    int32_t currentPos, targetPos;
    float currentSpeed;
    bool stepperEnabled;
    SystemState state;
    bool leftLimit, rightLimit;
    
    SAFE_READ_STATUS(currentPosition, currentPos);
    SAFE_READ_STATUS(targetPosition, targetPos);
    SAFE_READ_STATUS(currentSpeed, currentSpeed);
    SAFE_READ_STATUS(stepperEnabled, stepperEnabled);
    SAFE_READ_STATUS(systemState, state);
    
    // Get limit states from StepperController
    StepperController::getLimitStates(leftLimit, rightLimit);
    
    // Build status object
    doc["systemState"] = static_cast<int>(state);
    doc["position"]["current"] = currentPos;
    doc["position"]["target"] = targetPos;
    doc["speed"] = currentSpeed;
    doc["stepperEnabled"] = stepperEnabled;
    doc["limits"]["left"] = leftLimit;
    doc["limits"]["right"] = rightLimit;
    doc["isHoming"] = StepperController::isHoming();
    doc["isMoving"] = StepperController::isMoving();
    doc["isHomed"] = StepperController::isHomed();
    doc["limitFaultActive"] = StepperController::isLimitFaultActive();
    
    // Add position limits if homed
    int32_t minPos, maxPos;
    if (StepperController::getPositionLimits(minPos, maxPos)) {
        doc["positionLimits"]["min"] = minPos;
        doc["positionLimits"]["max"] = maxPos;
        doc["positionLimits"]["range"] = maxPos - minPos;
    }
    
    // Add basic config for UI
    SystemConfig* config = SystemConfigMgr::getConfig();
    doc["config"]["maxSpeed"] = config->defaultProfile.maxSpeed;
    doc["config"]["acceleration"] = config->defaultProfile.acceleration;
    doc["config"]["homingSpeed"] = config->homingSpeed;
    doc["config"]["jerk"] = config->defaultProfile.jerk;
    doc["config"]["emergencyDeceleration"] = config->emergencyDeceleration;
    doc["config"]["dmxChannel"] = config->dmxStartChannel;
    doc["config"]["dmxScale"] = config->dmxScale;
    doc["config"]["dmxOffset"] = config->dmxOffset;
    doc["config"]["dmxTimeout"] = config->dmxTimeout;
    doc["config"]["minPosition"] = config->minPosition;
    doc["config"]["maxPosition"] = config->maxPosition;
    doc["config"]["homePosition"] = config->homePosition;
    
    // Add system information
    doc["systemInfo"]["version"] = "4.1.0";
    doc["systemInfo"]["hardware"] = "ESP32-S3-WROOM-1";
    doc["systemInfo"]["uptime"] = millis();
    doc["systemInfo"]["freeHeap"] = ESP.getFreeHeap();
    doc["systemInfo"]["wifiClients"] = activeClients;
    doc["systemInfo"]["maxClients"] = WS_MAX_CLIENTS;
    
    // Get task stack high water mark for the broadcast task
    if (broadcastTaskHandle != nullptr) {
        doc["systemInfo"]["taskStackHighWaterMark"] = uxTaskGetStackHighWaterMark(broadcastTaskHandle);
    }
}

void WebInterface::getSystemConfig(JsonDocument& doc) {
    SystemConfig* config = SystemConfigMgr::getConfig();
    
    // Motion profile
    doc["motion"]["maxSpeed"] = config->defaultProfile.maxSpeed;
    doc["motion"]["acceleration"] = config->defaultProfile.acceleration;
    doc["motion"]["homingSpeed"] = config->homingSpeed;
    doc["motion"]["jerk"] = config->defaultProfile.jerk;
    
    // Position limits
    doc["limits"]["min"] = config->minPosition;
    doc["limits"]["max"] = config->maxPosition;
    doc["limits"]["home"] = config->homePosition;
    
    // DMX config
    doc["dmx"]["channel"] = config->dmxStartChannel;
    doc["dmx"]["scale"] = config->dmxScale;
    doc["dmx"]["offset"] = config->dmxOffset;
    doc["dmx"]["timeout"] = config->dmxTimeout;
    
    // Safety config
    doc["safety"]["emergencyDeceleration"] = config->emergencyDeceleration;
}

void WebInterface::getSystemInfo(JsonDocument& doc) {
    doc["version"] = "4.1.0";
    doc["hardware"] = "ESP32-S3-WROOM-1";
    doc["uptime"] = millis();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["clients"] = activeClients;
    doc["maxClients"] = WS_MAX_CLIENTS;
    
    // Task information
    if (webTaskHandle != nullptr) {
        doc["webTaskStackHighWaterMark"] = uxTaskGetStackHighWaterMark(webTaskHandle);
    }
    if (broadcastTaskHandle != nullptr) {
        doc["broadcastTaskStackHighWaterMark"] = uxTaskGetStackHighWaterMark(broadcastTaskHandle);
    }
    
    // WiFi information
    doc["apSSID"] = apSSID;
    doc["apIP"] = WiFi.softAPIP().toString();
    doc["apMAC"] = WiFi.softAPmacAddress();
    doc["apStations"] = WiFi.softAPgetStationNum();
}

bool WebInterface::sendMotionCommand(CommandType type, int32_t position) {
    MotionCommand cmd;
    cmd.type = type;
    cmd.timestamp = millis();
    cmd.commandId = nextCommandId++;
    
    if (type == CommandType::MOVE_ABSOLUTE || type == CommandType::MOVE_RELATIVE) {
        cmd.profile.targetPosition = position;
    }
    
    // Non-blocking send with no timeout
    return (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE);
}

bool WebInterface::updateConfiguration(const JsonDocument& params) {
    bool success = true;
    
    Serial.println("[WebInterface] Updating configuration...");
    
    // Get current config
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (!config) {
        Serial.println("[WebInterface] Failed to get config");
        return false;
    }
    
    // Track if motion parameters are updated for StepperController
    bool speedUpdated = false;
    bool accelUpdated = false;
    
    // Update all configuration values
    if (params.containsKey("maxSpeed")) {
        float speed = params["maxSpeed"];
        config->defaultProfile.maxSpeed = speed;
        speedUpdated = true;
        Serial.printf("[WebInterface] Setting maxSpeed to: %.1f\n", speed);
    }
    
    if (params.containsKey("acceleration")) {
        float accel = params["acceleration"];
        config->defaultProfile.acceleration = accel;
        config->defaultProfile.deceleration = accel;
        accelUpdated = true;
        Serial.printf("[WebInterface] Setting acceleration to: %.1f\n", accel);
    }
    
    if (params.containsKey("homingSpeed")) {
        float speed = params["homingSpeed"];
        config->homingSpeed = speed;
        Serial.printf("[WebInterface] Setting homingSpeed to: %.1f\n", speed);
    }
    
    if (params.containsKey("jerk")) {
        float jerk = params["jerk"];
        config->defaultProfile.jerk = jerk;
        Serial.printf("[WebInterface] Setting jerk to: %.1f\n", jerk);
    }
    
    if (params.containsKey("emergencyDeceleration")) {
        float emergDecel = params["emergencyDeceleration"];
        config->emergencyDeceleration = emergDecel;
        Serial.printf("[WebInterface] Setting emergencyDeceleration to: %.1f\n", emergDecel);
    }
    
    // Update DMX parameters
    if (params.containsKey("dmxChannel")) {
        config->dmxStartChannel = params["dmxChannel"];
        Serial.printf("[WebInterface] Setting dmxChannel to: %d\n", config->dmxStartChannel);
    }
    
    if (params.containsKey("dmxScale")) {
        config->dmxScale = params["dmxScale"];
        Serial.printf("[WebInterface] Setting dmxScale to: %.3f\n", config->dmxScale);
    }
    
    if (params.containsKey("dmxOffset")) {
        config->dmxOffset = params["dmxOffset"];
        Serial.printf("[WebInterface] Setting dmxOffset to: %d\n", config->dmxOffset);
    }
    
    if (params.containsKey("dmxTimeout")) {
        uint32_t timeout = params["dmxTimeout"];
        config->dmxTimeout = timeout;
        Serial.printf("[WebInterface] Setting dmxTimeout to: %u\n", timeout);
    }
    
    // Update position limits (these are usually set by homing, but allow manual override)
    if (params.containsKey("minPosition")) {
        config->minPosition = params["minPosition"];
        Serial.printf("[WebInterface] Setting minPosition to: %d\n", config->minPosition);
    }
    
    if (params.containsKey("maxPosition")) {
        config->maxPosition = params["maxPosition"];
        Serial.printf("[WebInterface] Setting maxPosition to: %d\n", config->maxPosition);
    }
    
    if (params.containsKey("homePosition")) {
        config->homePosition = params["homePosition"];
        Serial.printf("[WebInterface] Setting homePosition to: %d\n", config->homePosition);
    }
    
    // Save to flash
    if (SystemConfigMgr::saveToEEPROM()) {
        Serial.println("[WebInterface] Configuration saved to flash");
        
        // Now update StepperController with any motion changes
        if (speedUpdated) {
            if (!StepperController::setMaxSpeed(config->defaultProfile.maxSpeed)) {
                Serial.println("[WebInterface] Warning: Failed to update StepperController maxSpeed");
            }
        }
        
        if (accelUpdated) {
            if (!StepperController::setAcceleration(config->defaultProfile.acceleration)) {
                Serial.println("[WebInterface] Warning: Failed to update StepperController acceleration");
            }
        }
    } else {
        Serial.println("[WebInterface] Failed to save configuration to flash");
        success = false;
    }
    
    return success;
}

void WebInterface::broadcastStatus() {
    StaticJsonDocument<512> doc;
    getSystemStatus(doc);
    
    String message;
    serializeJson(doc, message);
    
    // Send to all connected WebSocket clients
    for (uint8_t i = 0; i < WS_MAX_CLIENTS; i++) {
        if (clientConnected[i]) {
            wsServer->sendTXT(i, message);
        }
    }
}

void WebInterface::sendStatusToClient(uint8_t num) {
    StaticJsonDocument<512> doc;
    getSystemStatus(doc);
    
    String message;
    serializeJson(doc, message);
    
    wsServer->sendTXT(num, message);
}

bool WebInterface::validateCommand(const JsonDocument& cmd) {
    if (!cmd.containsKey("command")) {
        return false;
    }
    
    String command = cmd["command"];
    
    // Check system state for safety
    SystemState state;
    SAFE_READ_STATUS(systemState, state);
    
    if (state == SystemState::EMERGENCY_STOP) {
        // Only allow emergency clear commands
        return (command == "clear_emergency" || command == "estop");
    }
    
    // Validate move commands
    if (command == "move" && cmd.containsKey("position")) {
        int32_t pos = cmd["position"];
        SystemConfig* config = SystemConfigMgr::getConfig();
        return (pos >= config->minPosition && 
                pos <= config->maxPosition);
    }
    
    return true;
}

String WebInterface::buildJsonResponse(const char* status, const char* message) {
    StaticJsonDocument<128> doc;
    doc["status"] = status;
    doc["message"] = message;
    
    String response;
    serializeJson(doc, response);
    return response;
}

// ============================================================================
// Response Helpers
// ============================================================================

void WebInterface::sendJsonResponse(int code, const JsonDocument& doc) {
    String response;
    serializeJson(doc, response);
    httpServer->send(code, "application/json", response);
}

void WebInterface::sendJsonResponse(int code, const char* status, const char* message) {
    httpServer->send(code, "application/json", buildJsonResponse(status, message));
}

// ============================================================================
// Public Getters
// ============================================================================

String WebInterface::getAPAddress() const {
    if (running) {
        return WiFi.softAPIP().toString();
    }
    return "Not running";
}

void WebInterface::setCredentials(const char* ssid, const char* password) {
    apSSID = ssid;
    apPassword = password;
}

void WebInterface::setEnabled(bool enable) {
    enabled = enable;
    if (!enable && running) {
        stop();
    }
}

#endif // ENABLE_WEB_INTERFACE