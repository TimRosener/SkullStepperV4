/*
 * WebInterface.cpp
 * 
 * Implementation using built-in ESP32 WebServer and WebSocketsServer
 * All content served from memory - no filesystem dependencies
 */

#include "WebInterface.h"
#include "StepperController.h"  // Ensure StepperController interface is included
#include "SerialInterface.h"    // For processCommand function
#include "DMXReceiver.h"        // For DMX status information
#include "InputValidation.h"    // For input bounds checking
#include <esp_random.h>         // For esp_random() function
#include <esp_system.h>         // For esp_reset_reason()

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
            <h2>Status</h2>
            
            <!-- Status tab buttons -->
            <div class="status-tabs">
                <button class="tab-btn active" onclick="showStatusTab('system', event)">System Status</button>
                <button class="tab-btn" onclick="showStatusTab('dmx', event)">DMX Status</button>
                <button class="tab-btn" onclick="showStatusTab('diagnostics', event)">Diagnostics</button>
            </div>
            
            <!-- System Status Tab -->
            <div id="system-status-tab" class="status-tab active">
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
            
            <!-- DMX Status Tab -->
            <div id="dmx-status-tab" class="status-tab">
                <div class="status-grid">
                    <div class="status-item">
                        <label>DMX Active:</label>
                        <span id="dmxActive" class="value">--</span>
                    </div>
                    <div class="status-item">
                        <label>DMX Start Ch:</label>
                        <span id="dmxOffset" class="value">--</span>
                    </div>
                    <div class="status-item">
                        <label>Ch1 (Position):</label>
                        <span id="dmxCh1" class="value">--</span>
                        <span id="dmxPos" class="calc-text">--</span>
                    </div>
                    <div class="status-item">
                        <label>Ch2 (Fine):</label>
                        <span id="dmxCh2" class="value">--</span>
                    </div>
                    <div class="status-item">
                        <label>Ch3 (Acceleration):</label>
                        <span id="dmxCh3" class="value">--</span>
                        <span id="dmxAccel" class="calc-text">--</span>
                    </div>
                    <div class="status-item">
                        <label>Ch4 (Speed):</label>
                        <span id="dmxCh4" class="value">--</span>
                        <span id="dmxSpeed" class="calc-text">--</span>
                    </div>
                    <div class="status-item">
                        <label>Ch5 (Mode):</label>
                        <span id="dmxCh5" class="value">--</span>
                        <span id="dmxMode" class="mode-text">--</span>
                    </div>
                </div>
            </div>
            
            <!-- Diagnostics Tab -->
            <div id="diagnostics-status-tab" class="status-tab">
                <div class="diagnostics-grid">
                    <div class="diag-section">
                        <h4>Memory Status</h4>
                        <div class="diag-item">
                            <label>Free Heap:</label>
                            <span id="diagFreeHeap" class="value">--</span>
                            <span id="diagHeapPercent" class="percent-text">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Total Heap:</label>
                            <span id="diagTotalHeap" class="value">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Min Free:</label>
                            <span id="diagMinFree" class="value">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Largest Block:</label>
                            <span id="diagMaxBlock" class="value">--</span>
                        </div>
                    </div>
                    
                    <div class="diag-section">
                        <h4>Task Health</h4>
                        <div class="diag-item">
                            <label>StepperCtrl [Core 0]:</label>
                            <span id="taskStepperStatus" class="task-status">--</span>
                        </div>
                        <div class="diag-item">
                            <label>DMXReceiver [Core 0]:</label>
                            <span id="taskDMXStatus" class="task-status">--</span>
                        </div>
                        <div class="diag-item">
                            <label>WebServer [Core 1]:</label>
                            <span id="taskWebStatus" class="task-status">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Broadcast [Core 1]:</label>
                            <span id="taskBroadcastStatus" class="task-status">--</span>
                        </div>
                    </div>
                    
                    <div class="diag-section">
                        <h4>System Information</h4>
                        <div class="diag-item">
                            <label>CPU Model:</label>
                            <span id="diagCPUModel" class="value">--</span>
                        </div>
                        <div class="diag-item">
                            <label>CPU Frequency:</label>
                            <span id="diagCPUFreq" class="value">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Flash Size:</label>
                            <span id="diagFlashSize" class="value">--</span>
                        </div>
                        <div class="diag-item">
                            <label>Reset Reason:</label>
                            <span id="diagResetReason" class="value">--</span>
                        </div>
                    </div>
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
                <div style="margin-top: 10px; text-align: center;">
                    <button class="btn btn-success" onclick="moveToHome()" title="Move to configured home position">MOVE TO HOME</button>
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
                <p class="test-info">Use STOP or E-STOP buttons to stop tests.</p>
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
                <button class="tab-btn active" onclick="showConfigTab('motion')">Motion & Limits</button>
                <button class="tab-btn" onclick="showConfigTab('dmx')">DMX</button>
            </div>
            
            <!-- Motion Configuration Tab - now includes Position Limits -->
            <div id="motion-tab" class="config-tab active">
                <h3>Motion Parameters</h3>
                <div id="motionParams">
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
                    <div class="config-item">
                        <label for="limitSafetyMargin">Limit Safety Margin:</label>
                        <input type="range" id="limitSafetyMargin" min="0" max="1000" step="10">
                        <span id="limitSafetyMarginValue">--</span> steps
                        <small class="param-info">Distance to stay away from limit switches (0-1000 steps)</small>
                    </div>
                </div>
                
                <h3 style="margin-top: 25px;">Position Limits</h3>
                <div id="limitsContent" style="display:none;">
                    <div class="limits-info" style="background: rgba(0,212,255,0.1); padding: 15px; border-radius: 5px; margin-bottom: 15px;">
                        <p style="margin: 0 0 10px 0;"><strong>Detected Physical Range:</strong></p>
                        <p style="margin: 0;">Min: <span id="detectedMin" style="color: #00d4ff; font-weight: bold;">--</span> steps</p>
                        <p style="margin: 0;">Max: <span id="detectedMax" style="color: #00d4ff; font-weight: bold;">--</span> steps</p>
                        <p style="margin: 10px 0 0 0;">Total Range: <span id="detectedRange" style="color: #00d4ff; font-weight: bold;">--</span> steps</p>
                    </div>
                    <div class="config-item">
                        <label for="minPositionPercent">Minimum Position:</label>
                        <input type="range" id="minPositionPercent" min="0" max="45" step="5" value="0">
                        <span id="minPositionPercentValue">0</span>% of range
                        <small class="param-info">Safety margin from left limit (0-45%)</small>
                    </div>
                    <div class="config-item">
                        <label for="maxPositionPercent">Maximum Position:</label>
                        <input type="range" id="maxPositionPercent" min="55" max="100" step="5" value="100">
                        <span id="maxPositionPercentValue">100</span>% of range
                        <small class="param-info">Safety margin from right limit (55-100%)</small>
                    </div>
                    <div class="config-item">
                        <label for="homePositionPercent">Home Position:</label>
                        <input type="range" id="homePositionPercent" min="0" max="100" step="5" value="50">
                        <span id="homePositionPercentValue">50</span>% of range
                        <small class="param-info">Position to return to after homing (0% = left limit, 100% = right limit)</small>
                    </div>
                </div>
                
                <h3 style="margin-top: 25px;">Advanced Motion Settings</h3>
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
                
                <h3 style="margin-top: 25px;">Homing Options</h3>
                <div class="config-item">
                    <label style="display: flex; align-items: center; cursor: pointer;">
                        <input type="checkbox" id="autoHomeOnBoot" style="margin-right: 10px; width: auto;">
                        Auto-Home on Boot
                    </label>
                    <small class="param-info">Automatically perform homing sequence when system starts up</small>
                </div>
                <div class="config-item">
                    <label style="display: flex; align-items: center; cursor: pointer;">
                        <input type="checkbox" id="autoHomeOnEstop" style="margin-right: 10px; width: auto;">
                        Auto-Home on E-Stop
                    </label>
                    <small class="param-info">Automatically re-home after emergency stop or unexpected limit switch activation</small>
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
                    <label for="dmxTimeout">DMX Timeout:</label>
                    <input type="number" id="dmxTimeout" min="100" max="60000" step="100" placeholder="Milliseconds">
                    <small class="param-info">Time before DMX signal loss is detected (100-60000 ms)</small>
                </div>
            </div>

            
            <div style="margin: 15px 0;">
                <label style="display: flex; align-items: center; gap: 10px;">
                    <input type="checkbox" id="livePreview" onchange="toggleLivePreview()">
                    <span>Live Preview - Adjust speed/acceleration in real-time during motion (changes not saved to flash)</span>
                </label>
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

/* DMX-specific styling */
#dmxOffset {
    color: var(--warning-color);
}

.mode-text {
    margin-left: 10px;
    font-size: 0.85em;
    color: var(--text-dim);
    font-style: italic;
}

.calc-text {
    margin-left: 10px;
    font-size: 0.85em;
    color: var(--primary-color);
    font-weight: normal;
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

.btn-success:hover {
    background: #00cc66;
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

.status-tabs {
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

.status-tab {
    display: none;
    animation: fadeIn 0.3s;
}

.status-tab.active {
    display: block;
}

@keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

/* Diagnostics tab styling */
.diagnostics-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 20px;
}

.diag-section {
    background: rgba(0, 0, 0, 0.3);
    border-radius: 8px;
    padding: 15px;
    border: 1px solid var(--border-color);
}

.diag-section h4 {
    color: var(--primary-color);
    margin: 0 0 15px 0;
    font-size: 1.1em;
    border-bottom: 1px solid var(--border-color);
    padding-bottom: 8px;
}

.diag-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 6px 0;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
}

.diag-item:last-child {
    border-bottom: none;
}

.diag-item label {
    color: var(--text-dim);
    font-size: 0.9em;
}

.diag-item .value {
    color: var(--primary-color);
    font-weight: bold;
    font-size: 0.9em;
}

.percent-text {
    color: var(--success-color);
    font-size: 0.85em;
    margin-left: 10px;
}

.task-status {
    font-size: 0.85em;
    font-weight: bold;
}

.task-status.running {
    color: var(--success-color);
}

.task-status.error {
    color: var(--danger-color);
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
let detectedLimits = null;  // Store detected position limits from homing
let livePreviewEnabled = false;  // Track if live preview mode is active

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

// Helper function to get reset reason string
function getResetReasonString(reason) {
    const reasons = {
        1: 'Power On',
        3: 'Software Reset',
        4: 'Watchdog Reset',
        5: 'Deep Sleep',
        6: 'Brown Out',
        7: 'SDIO Reset'
    };
    return reasons[reason] || 'Unknown';
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
    
    // Update DMX status
    if (data.dmx) {
        const dmxActiveEl = document.getElementById('dmxActive');
        dmxActiveEl.textContent = data.dmx.active ? 'YES' : 'NO';
        dmxActiveEl.style.color = data.dmx.active ? 'var(--success-color)' : 'var(--text-dim)';
        
        document.getElementById('dmxOffset').textContent = data.dmx.offset || '0';
        
        // Update channel values
        if (data.dmx.channels) {
            // Channel 1 & 2: Position (with calculations if we have position limits)
            const ch1Value = data.dmx.channels[0] || 0;
            const ch2Value = data.dmx.channels[1] || 0;
            document.getElementById('dmxCh1').textContent = ch1Value;
            document.getElementById('dmxCh2').textContent = ch2Value;
            
            // Calculate position in steps
            if (data.positionLimits && data.positionLimits.valid && data.config) {
                // DMX position is always percentage-based in this system
                const positionPercent = (ch1Value / 255.0) * 100.0;
                
                // Use configured position limits
                const minPos = data.config.minPosition || data.positionLimits.min;
                const maxPos = data.config.maxPosition || data.positionLimits.max;
                const range = maxPos - minPos;
                const targetPosition = minPos + Math.round((range * positionPercent) / 100.0);
                
                document.getElementById('dmxPos').textContent = `(${targetPosition} steps)`;
            } else {
                document.getElementById('dmxPos').textContent = '';
            }
            
            // Channel 3: Acceleration
            const ch3Value = data.dmx.channels[2] || 0;
            document.getElementById('dmxCh3').textContent = ch3Value;
            if (data.config && data.config.acceleration) {
                const accelPercent = (ch3Value / 255.0) * 100.0;
                const accelValue = Math.round((data.config.acceleration * accelPercent) / 100.0);
                document.getElementById('dmxAccel').textContent = `(${accelValue} steps/s²)`;
            } else {
                document.getElementById('dmxAccel').textContent = '';
            }
            
            // Channel 4: Speed
            const ch4Value = data.dmx.channels[3] || 0;
            document.getElementById('dmxCh4').textContent = ch4Value;
            if (data.config && data.config.maxSpeed) {
                const speedPercent = (ch4Value / 255.0) * 100.0;
                const speedValue = Math.round((data.config.maxSpeed * speedPercent) / 100.0);
                document.getElementById('dmxSpeed').textContent = `(${speedValue} steps/s)`;
            } else {
                document.getElementById('dmxSpeed').textContent = '';
            }
            
            // Channel 5 with mode translation
            const ch5Value = data.dmx.channels[4] || 0;
            document.getElementById('dmxCh5').textContent = ch5Value;
            
            // Decode mode based on DMXReceiver.h thresholds:
            // 0-84: STOP, 85-170: CONTROL, 171-255: HOME
            let modeText = '';
            let modeColor = '';
            if (ch5Value <= 84) {
                modeText = '(STOP)';
                modeColor = 'var(--danger-color)';
            } else if (ch5Value <= 170) {
                modeText = '(CONTROL)';
                modeColor = 'var(--success-color)';
            } else {
                modeText = '(HOME)';
                modeColor = 'var(--warning-color)';
            }
            const dmxModeEl = document.getElementById('dmxMode');
            dmxModeEl.textContent = modeText;
            dmxModeEl.style.color = modeColor;
        }
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
    
    // Update diagnostics tab if data is available
    if (data.diagnostics) {
        // Memory information
        if (data.diagnostics.memory) {
            const mem = data.diagnostics.memory;
            document.getElementById('diagFreeHeap').textContent = formatMemory(mem.freeHeap);
            document.getElementById('diagTotalHeap').textContent = formatMemory(mem.totalHeap);
            document.getElementById('diagMinFree').textContent = formatMemory(mem.minFreeHeap);
            document.getElementById('diagMaxBlock').textContent = formatMemory(mem.maxAllocHeap);
            
            // Calculate percentage
            const percent = ((mem.freeHeap / mem.totalHeap) * 100).toFixed(1);
            const percentEl = document.getElementById('diagHeapPercent');
            percentEl.textContent = `(${percent}%)`;
            
            // Color code based on percentage
            if (percent < 20) {
                percentEl.style.color = 'var(--danger-color)';
            } else if (percent < 40) {
                percentEl.style.color = 'var(--warning-color)';
            } else {
                percentEl.style.color = 'var(--success-color)';
            }
        }
        
        // Task information
        if (data.diagnostics.tasks) {
            const tasks = data.diagnostics.tasks;
            
            // StepperController
            const stepperEl = document.getElementById('taskStepperStatus');
            if (tasks.stepperExists) {
                stepperEl.textContent = `✓ Running (Stack: ${tasks.stepperStack || 'N/A'} bytes free)`;
                stepperEl.className = 'task-status running';
            } else {
                stepperEl.textContent = '✗ Not running';
                stepperEl.className = 'task-status error';
            }
            
            // DMXReceiver
            const dmxEl = document.getElementById('taskDMXStatus');
            if (tasks.dmxExists) {
                dmxEl.textContent = `✓ Running (Stack: ${tasks.dmxStack || 'N/A'} bytes free)`;
                dmxEl.className = 'task-status running';
            } else {
                dmxEl.textContent = '✗ Not running';
                dmxEl.className = 'task-status error';
            }
            
            // WebServer
            const webEl = document.getElementById('taskWebStatus');
            if (tasks.webExists) {
                webEl.textContent = `✓ Running (Stack: ${tasks.webStack || 'N/A'} bytes free)`;
                webEl.className = 'task-status running';
            } else {
                webEl.textContent = '✗ Not running';
                webEl.className = 'task-status error';
            }
            
            // Broadcast
            const broadcastEl = document.getElementById('taskBroadcastStatus');
            if (tasks.broadcastExists) {
                broadcastEl.textContent = `✓ Running (Stack: ${tasks.broadcastStack || 'N/A'} bytes free)`;
                broadcastEl.className = 'task-status running';
            } else {
                broadcastEl.textContent = '✗ Not running';
                broadcastEl.className = 'task-status error';
            }
        }
        
        // System information
        if (data.diagnostics.system) {
            const sys = data.diagnostics.system;
            document.getElementById('diagCPUModel').textContent = 'ESP32-S3';
            document.getElementById('diagCPUFreq').textContent = `${sys.cpuFreq || 240} MHz`;
            document.getElementById('diagFlashSize').textContent = formatMemory(sys.flashSize || 0);
            document.getElementById('diagResetReason').textContent = getResetReasonString(sys.resetReason || 0);
        }
    }
    
    // Handle position limits display
    if (data.positionLimits) {
        if (data.detectedLimits && data.detectedLimits.valid) {
            // Store actual physical detected limits (where switches are)
            detectedLimits = {
                min: data.detectedLimits.left,
                max: data.detectedLimits.right,
                range: data.detectedLimits.range
            };
            
            // Update limits display
            updateLimitsDisplay();
        } else {
            detectedLimits = null;
            updateLimitsDisplay();
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
            // Also update the duplicate slider if it exists
            const alsoSlider = document.getElementById('homingSpeedAlso');
            if (alsoSlider) {
                alsoSlider.value = data.config.homingSpeed;
                document.getElementById('homingSpeedAlsoValue').textContent = data.config.homingSpeed;
            }
        }
        if (data.config.limitSafetyMargin !== undefined) {
            document.getElementById('limitSafetyMargin').value = data.config.limitSafetyMargin;
            document.getElementById('limitSafetyMarginValue').textContent = data.config.limitSafetyMargin;
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
        if (data.config.dmxTimeout !== undefined) {
            document.getElementById('dmxTimeout').value = data.config.dmxTimeout;
        }
        
        // Position limits - convert from steps to percentages if we have detected limits
        if (detectedLimits && data.config.minPosition !== undefined && data.config.maxPosition !== undefined) {
            const range = detectedLimits.max - detectedLimits.min;
            const minPercent = Math.round(((data.config.minPosition - detectedLimits.min) / range) * 100);
            const maxPercent = Math.round(((data.config.maxPosition - detectedLimits.min) / range) * 100);
            
            document.getElementById('minPositionPercent').value = minPercent;
            document.getElementById('minPositionPercentValue').textContent = minPercent;
            document.getElementById('maxPositionPercent').value = maxPercent;
            document.getElementById('maxPositionPercentValue').textContent = maxPercent;
        }
        if (data.config.homePositionPercent !== undefined) {
            document.getElementById('homePositionPercent').value = data.config.homePositionPercent;
            document.getElementById('homePositionPercentValue').textContent = data.config.homePositionPercent;
        }
        // Update auto-home checkboxes
        if (data.config.autoHomeOnBoot !== undefined) {
            document.getElementById('autoHomeOnBoot').checked = data.config.autoHomeOnBoot;
        }
        if (data.config.autoHomeOnEstop !== undefined) {
            document.getElementById('autoHomeOnEstop').checked = data.config.autoHomeOnEstop;
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

function moveToHome() {
    // Check if system is homed and we have detected limits
    if (!detectedLimits) {
        alert('System must be homed before moving to home position');
        return;
    }
    
    // Get the configured home position percentage
    const homePercent = parseFloat(document.getElementById('homePositionPercent').value);
    
    // Calculate the actual position based on percentage
    const range = detectedLimits.max - detectedLimits.min;
    const homePosition = detectedLimits.min + Math.floor((range * homePercent) / 100);
    
    // Send move command to calculated home position
    sendCommand('move', { position: homePosition });
    
    // Optional: Show feedback
    console.log(`Moving to home position: ${homePosition} (${homePercent}% of range)`);
}

function jog(steps) {
    sendCommand('jog', { steps: steps });
}

function toggleLivePreview() {
    livePreviewEnabled = document.getElementById('livePreview').checked;
    
    if (livePreviewEnabled) {
        console.log('Live preview enabled - changes will apply immediately');
        // Show a notification
        const msg = document.createElement('div');
        msg.style.cssText = 'position: fixed; top: 20px; right: 20px; background: #00ff88; color: black; padding: 10px 20px; border-radius: 5px; z-index: 1000;';
        msg.textContent = 'Live Preview ON - Changes apply immediately';
        document.body.appendChild(msg);
        setTimeout(() => msg.remove(), 3000);
    } else {
        console.log('Live preview disabled - use Apply Changes to save');
        // Show a notification
        const msg = document.createElement('div');
        msg.style.cssText = 'position: fixed; top: 20px; right: 20px; background: #ffaa00; color: black; padding: 10px 20px; border-radius: 5px; z-index: 1000;';
        msg.textContent = 'Live Preview OFF - Use Apply Changes to save';
        document.body.appendChild(msg);
        setTimeout(() => msg.remove(), 3000);
    }
}

function applyConfig() {
    // Determine which tab is currently active
    const activeTab = document.querySelector('.config-tab.active').id;
    const config = {};
    
    if (activeTab === 'motion-tab') {
        // Motion & Limits tab
        // Always include all motion parameters - configurations can be changed anytime
        config.maxSpeed = parseInt(document.getElementById('maxSpeed').value);
        config.acceleration = parseInt(document.getElementById('acceleration').value);
        // Get homing speed from whichever slider is visible
        const homingSpeedAlso = document.getElementById('homingSpeedAlso');
        if (homingSpeedAlso && homingSpeedAlso.offsetParent !== null) {
            config.homingSpeed = parseInt(homingSpeedAlso.value);
        } else {
            config.homingSpeed = parseInt(document.getElementById('homingSpeed').value);
        }
        
        // Limit safety margin
        config.limitSafetyMargin = parseInt(document.getElementById('limitSafetyMargin').value);
        
        // Position limits - convert percentages to actual positions if homed
        if (detectedLimits) {
            const minPercent = parseFloat(document.getElementById('minPositionPercent').value);
            const maxPercent = parseFloat(document.getElementById('maxPositionPercent').value);
            const range = detectedLimits.max - detectedLimits.min;
            
            config.minPosition = detectedLimits.min + Math.floor((range * minPercent) / 100);
            config.maxPosition = detectedLimits.min + Math.floor((range * maxPercent) / 100);
            config.homePositionPercent = parseFloat(document.getElementById('homePositionPercent').value);
            
            // Validate the resulting range
            if (config.maxPosition - config.minPosition < 100) {
                alert('Position range must be at least 100 steps. Please adjust the percentages.');
                return;
            }
        }
        
        // Include auto-home settings
        config.autoHomeOnBoot = document.getElementById('autoHomeOnBoot').checked;
        config.autoHomeOnEstop = document.getElementById('autoHomeOnEstop').checked;
        
        // Include advanced motion settings (now part of Motion & Limits tab)
        config.jerk = parseInt(document.getElementById('jerk').value);
        config.emergencyDeceleration = parseInt(document.getElementById('emergencyDeceleration').value);
    } else if (activeTab === 'dmx-tab') {
        // DMX tab - only channel and timeout now
        config.dmxChannel = parseInt(document.getElementById('dmxChannel').value);
        config.dmxTimeout = parseInt(document.getElementById('dmxTimeout').value);
    }
    
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
    
    // Update limits display when motion tab is selected (since it now contains limits)
    if (tabName === 'motion') {
        updateLimitsDisplay();
    }
}

function showStatusTab(tabName, event) {
    // Hide all status tabs
    document.querySelectorAll('.status-tab').forEach(tab => {
        tab.classList.remove('active');
    });
    
    // Remove active class from status tab buttons only
    document.querySelectorAll('.status-tabs .tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });
    
    // Show selected tab
    document.getElementById(tabName + '-status-tab').classList.add('active');
    
    // Add active class to clicked button
    if (event && event.target) {
        event.target.classList.add('active');
    }
}

// Initialize sliders with interaction tracking
document.getElementById('homePositionPercent').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('homePositionPercentValue').textContent = e.target.value;
});

document.getElementById('minPositionPercent').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    const value = parseInt(e.target.value);
    document.getElementById('minPositionPercentValue').textContent = value;
    
    // Ensure max is at least 10% higher than min
    const maxSlider = document.getElementById('maxPositionPercent');
    const minAllowedMax = value + 10;
    if (parseInt(maxSlider.value) < minAllowedMax) {
        maxSlider.value = minAllowedMax;
        document.getElementById('maxPositionPercentValue').textContent = minAllowedMax;
    }
});

document.getElementById('maxPositionPercent').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    const value = parseInt(e.target.value);
    document.getElementById('maxPositionPercentValue').textContent = value;
    
    // Ensure min is at least 10% lower than max
    const minSlider = document.getElementById('minPositionPercent');
    const maxAllowedMin = value - 10;
    if (parseInt(minSlider.value) > maxAllowedMin) {
        minSlider.value = maxAllowedMin;
        document.getElementById('minPositionPercentValue').textContent = maxAllowedMin;
    }
});

document.getElementById('maxSpeed').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('maxSpeedValue').textContent = e.target.value;
    
    // Send live update if live preview is enabled
    if (livePreviewEnabled) {
        const config = {
            maxSpeed: parseInt(e.target.value),
            live: true  // Mark as live update (no flash save)
        };
        sendCommand('config', { params: config });
    }
});

document.getElementById('acceleration').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('accelerationValue').textContent = e.target.value;
    
    // Send live update if live preview is enabled
    if (livePreviewEnabled) {
        const config = {
            acceleration: parseInt(e.target.value),
            live: true  // Mark as live update (no flash save)
        };
        sendCommand('config', { params: config });
    }
});

document.getElementById('homingSpeed').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('homingSpeedValue').textContent = e.target.value;
    // Also update the duplicate slider if it exists
    const alsoSlider = document.getElementById('homingSpeedAlso');
    if (alsoSlider) {
        alsoSlider.value = e.target.value;
        document.getElementById('homingSpeedAlsoValue').textContent = e.target.value;
    }
});

document.getElementById('limitSafetyMargin').addEventListener('input', (e) => {
    isAdjustingSliders = true;
    document.getElementById('limitSafetyMarginValue').textContent = e.target.value;
});

// Removed duplicate slider listener - now using single set of controls

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

if (document.getElementById('homingSpeedAlso')) {
    document.getElementById('homingSpeedAlso').addEventListener('mousedown', () => {
        isAdjustingSliders = true;
    });
}

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

// Removed duplicate slider touch listener

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

// Update limits display based on homing status
function updateLimitsDisplay() {
    const limitsContent = document.getElementById('limitsContent');
    const minPercentInput = document.getElementById('minPositionPercent');
    const maxPercentInput = document.getElementById('maxPositionPercent');
    const homePercentInput = document.getElementById('homePositionPercent');
    
    // Update position limits visibility based on homing state
    if (detectedLimits) {
        // System is homed - show position limits section
        limitsContent.style.display = 'block';
        
        // Update detected values display
        document.getElementById('detectedMin').textContent = detectedLimits.min;
        document.getElementById('detectedMax').textContent = detectedLimits.max;
        document.getElementById('detectedRange').textContent = detectedLimits.range;
        
        // Enable inputs
        minPercentInput.disabled = false;
        maxPercentInput.disabled = false;
        homePercentInput.disabled = false;
    } else {
        // System not homed - hide position limits section
        limitsContent.style.display = 'none';
        
        // Disable position limit inputs only
        minPercentInput.disabled = true;
        maxPercentInput.disabled = true;
        homePercentInput.disabled = true;
    }
}

// Helper functions for warnings and control state
function showHomingRequired() {
    // Find or create warning div
    let warning = document.getElementById('homingWarning');
    if (!warning) {
        warning = document.createElement('div');
        warning.id = 'homingWarning';
        warning.style.cssText = 'background: #ff6600; color: white; padding: 15px; text-align: center; font-weight: bold; margin: 20px 0; border-radius: 5px;';
        warning.innerHTML = '⚠️ HOMING REQUIRED - No movement is allowed until homing is completed, but all the configurations can be changed.';
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
    
    const moveHomeBtn = document.querySelector('button[onclick="moveToHome()"]');
    if (moveHomeBtn) {
        moveHomeBtn.disabled = true;
        moveHomeBtn.style.opacity = '0.5';
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
    
    // Note: We do NOT disable configuration controls - they can be changed anytime
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
    
    const moveHomeBtn = document.querySelector('button[onclick="moveToHome()"]');
    if (moveHomeBtn) {
        moveHomeBtn.disabled = false;
        moveHomeBtn.style.opacity = '1';
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
    // Initialize limits display
    updateLimitsDisplay();
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
    
    // Add detected physical limits (actual switch positions)
    int32_t detectedLeft, detectedRight;
    if (StepperController::getDetectedLimits(detectedLeft, detectedRight)) {
        doc["detectedLimits"]["left"] = detectedLeft;
        doc["detectedLimits"]["right"] = detectedRight;
        doc["detectedLimits"]["range"] = detectedRight - detectedLeft;
        doc["detectedLimits"]["valid"] = true;
    } else {
        doc["detectedLimits"]["valid"] = false;
    }
    
    // Add operating position limits (includes safety margin)
    int32_t minPos, maxPos;
    if (StepperController::getPositionLimits(minPos, maxPos)) {
        doc["positionLimits"]["min"] = minPos;
        doc["positionLimits"]["max"] = maxPos;
        doc["positionLimits"]["range"] = maxPos - minPos;
        doc["positionLimits"]["valid"] = true;
    } else {
        doc["positionLimits"]["valid"] = false;
    }
    
    // Add basic config for UI
    SystemConfig* config = SystemConfigMgr::getConfig();
    doc["config"]["maxSpeed"] = config->defaultProfile.maxSpeed;
    doc["config"]["acceleration"] = config->defaultProfile.acceleration;
    doc["config"]["homingSpeed"] = config->homingSpeed;
    doc["config"]["limitSafetyMargin"] = config->limitSafetyMargin;
    doc["config"]["jerk"] = config->defaultProfile.jerk;
    doc["config"]["emergencyDeceleration"] = config->emergencyDeceleration;
    doc["config"]["dmxChannel"] = config->dmxStartChannel;
    doc["config"]["dmxTimeout"] = config->dmxTimeout;
    doc["config"]["minPosition"] = config->minPosition;
    doc["config"]["maxPosition"] = config->maxPosition;
    doc["config"]["homePositionPercent"] = config->homePositionPercent;
    doc["config"]["autoHomeOnBoot"] = config->autoHomeOnBoot;
    doc["config"]["autoHomeOnEstop"] = config->autoHomeOnEstop;
    
    // Add DMX information
    uint8_t dmxChannels[5] = {0};
    bool dmxActive = DMXReceiver::isSignalPresent();
    if (dmxActive) {
        DMXReceiver::getChannelCache(dmxChannels);
    }
    
    doc["dmx"]["active"] = dmxActive;
    doc["dmx"]["offset"] = DMXReceiver::getBaseChannel();  // Show the actual base channel
    JsonArray channels = doc["dmx"].createNestedArray("channels");
    for (int i = 0; i < 5; i++) {
        channels.add(dmxChannels[i]);
    }
    
    // Add system information
    doc["systemInfo"]["version"] = "4.1.13";
    doc["systemInfo"]["hardware"] = "ESP32-S3-WROOM-1";
    doc["systemInfo"]["uptime"] = millis();
    doc["systemInfo"]["freeHeap"] = ESP.getFreeHeap();
    doc["systemInfo"]["wifiClients"] = activeClients;
    doc["systemInfo"]["maxClients"] = WS_MAX_CLIENTS;
    
    // Get task stack high water mark for the broadcast task
    if (broadcastTaskHandle != nullptr) {
        doc["systemInfo"]["taskStackHighWaterMark"] = uxTaskGetStackHighWaterMark(broadcastTaskHandle);
    }
    
    // Add detailed diagnostics data
    JsonObject diag = doc.createNestedObject("diagnostics");
    
    // Memory diagnostics
    JsonObject memory = diag.createNestedObject("memory");
    memory["freeHeap"] = ESP.getFreeHeap();
    memory["totalHeap"] = ESP.getHeapSize();
    memory["minFreeHeap"] = ESP.getMinFreeHeap();
    memory["maxAllocHeap"] = ESP.getMaxAllocHeap();
    
    // Task diagnostics - get stack high water marks
    JsonObject tasks = diag.createNestedObject("tasks");
    
    // StepperController task - check actual task health
    tasks["stepperExists"] = StepperController::isTaskHealthy();
    tasks["stepperLastUpdate"] = StepperController::getLastTaskUpdateTime();
    // Stack info not available without access to task handle
    
    // DMXReceiver task - check actual task health
    tasks["dmxExists"] = DMXReceiver::isTaskHealthy();
    tasks["dmxLastUpdate"] = DMXReceiver::getLastTaskUpdateTime();
    TaskHandle_t dmxHandle = DMXReceiver::getTaskHandle();
    if (dmxHandle != nullptr) {
        tasks["dmxStack"] = uxTaskGetStackHighWaterMark(dmxHandle);
    }
    
    // WebInterface tasks
    if (webTaskHandle != nullptr) {
        tasks["webStack"] = uxTaskGetStackHighWaterMark(webTaskHandle);
        tasks["webExists"] = true;
    } else {
        tasks["webExists"] = false;
    }
    
    if (broadcastTaskHandle != nullptr) {
        tasks["broadcastStack"] = uxTaskGetStackHighWaterMark(broadcastTaskHandle);
        tasks["broadcastExists"] = true;
    } else {
        tasks["broadcastExists"] = false;
    }
    
    // System info
    JsonObject sysInfo = diag.createNestedObject("system");
    sysInfo["cpuFreq"] = ESP.getCpuFreqMHz();
    sysInfo["flashSize"] = ESP.getFlashChipSize();
    sysInfo["resetReason"] = esp_reset_reason();
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
    doc["limits"]["homePercent"] = config->homePositionPercent;
    
    // DMX config
    doc["dmx"]["channel"] = config->dmxStartChannel;
    doc["dmx"]["timeout"] = config->dmxTimeout;
    
    // Safety config
    doc["safety"]["emergencyDeceleration"] = config->emergencyDeceleration;
}

void WebInterface::getSystemInfo(JsonDocument& doc) {
    doc["version"] = "4.1.13";
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
    MotionCommand cmd = {};  // Zero-initialize to avoid random values
    cmd.type = type;
    cmd.timestamp = millis();
    cmd.commandId = nextCommandId++;
    
    // Get current motion profile from config to use proper speed/acceleration
    SystemConfig* config = SystemConfigMgr::getConfig();
    if (config) {
        cmd.profile = config->defaultProfile;  // Load saved speed, acceleration, etc.
    }
    
    // Override target position for movement commands
    if (type == CommandType::MOVE_ABSOLUTE || type == CommandType::MOVE_RELATIVE) {
        cmd.profile.targetPosition = position;
    }
    
    // Non-blocking send with no timeout
    return (xQueueSend(g_motionCommandQueue, &cmd, 0) == pdTRUE);
}

bool WebInterface::updateConfiguration(const JsonDocument& params) {
    bool success = true;
    
    // Check if this is a live update (no save to flash)
    bool liveUpdate = false;
    if (params.containsKey("live")) {
        liveUpdate = params["live"];
    }
    
    if (liveUpdate) {
        Serial.println("[WebInterface] Live parameter update (no flash save)...");
    } else {
        Serial.println("[WebInterface] Updating configuration...");
    }
    
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
        InputValidation::validateFloat(speed, ParamLimits::MIN_SPEED, 
                                      ParamLimits::MAX_SPEED, "maxSpeed");
        config->defaultProfile.maxSpeed = speed;
        speedUpdated = true;
        Serial.printf("[WebInterface] Setting maxSpeed to: %.1f\n", speed);
    }
    
    if (params.containsKey("acceleration")) {
        float accel = params["acceleration"];
        InputValidation::validateFloat(accel, ParamLimits::MIN_ACCELERATION,
                                      ParamLimits::MAX_ACCELERATION, "acceleration");
        config->defaultProfile.acceleration = accel;
        config->defaultProfile.deceleration = accel;
        accelUpdated = true;
        Serial.printf("[WebInterface] Setting acceleration to: %.1f\n", accel);
    }
    
    if (params.containsKey("homingSpeed")) {
        float speed = params["homingSpeed"];
        InputValidation::validateFloat(speed, ParamLimits::MIN_HOMING_SPEED,
                                      ParamLimits::MAX_HOMING_SPEED, "homingSpeed");
        config->homingSpeed = speed;
        Serial.printf("[WebInterface] Setting homingSpeed to: %.1f\n", speed);
    }
    
    if (params.containsKey("limitSafetyMargin")) {
        float margin = params["limitSafetyMargin"];
        InputValidation::validateFloat(margin, ParamLimits::MIN_LIMIT_MARGIN,
                                      ParamLimits::MAX_LIMIT_MARGIN, "limitSafetyMargin");
        config->limitSafetyMargin = margin;
        Serial.printf("[WebInterface] Setting limitSafetyMargin to: %.1f\n", margin);
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
    
    if (params.containsKey("dmxChannel")) {
        int32_t channel = params["dmxChannel"];
        InputValidation::validateInt32(channel, ParamLimits::MIN_DMX_CHANNEL,
                                      ParamLimits::MAX_DMX_CHANNEL, "dmxChannel");
        config->dmxStartChannel = channel;
        Serial.printf("[WebInterface] Setting dmxChannel to: %d\n", config->dmxStartChannel);
    }
    
    if (params.containsKey("dmxTimeout")) {
        int32_t timeout = params["dmxTimeout"];
        InputValidation::validateInt32(timeout, ParamLimits::MIN_DMX_TIMEOUT,
                                      ParamLimits::MAX_DMX_TIMEOUT, "dmxTimeout");
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
    
    if (params.containsKey("homePositionPercent")) {
        float percent = params["homePositionPercent"];
        // Validate percentage
        if (percent >= 0.0f && percent <= 100.0f) {
            config->homePositionPercent = percent;
            Serial.printf("[WebInterface] Setting homePositionPercent to: %.1f%%\n", percent);
        } else {
            Serial.printf("[WebInterface] Invalid homePositionPercent: %.1f%% (must be 0-100)\n", percent);
        }
    }
    
    // Update auto-home settings
    if (params.containsKey("autoHomeOnBoot")) {
        config->autoHomeOnBoot = params["autoHomeOnBoot"];
        Serial.printf("[WebInterface] Setting autoHomeOnBoot to: %s\n", config->autoHomeOnBoot ? "ON" : "OFF");
    }
    
    if (params.containsKey("autoHomeOnEstop")) {
        config->autoHomeOnEstop = params["autoHomeOnEstop"];
        Serial.printf("[WebInterface] Setting autoHomeOnEstop to: %s\n", config->autoHomeOnEstop ? "ON" : "OFF");
    }
    
    // For live updates, skip saving to flash
    if (liveUpdate) {
        // Just update StepperController with motion changes
        if (speedUpdated) {
            if (!StepperController::setMaxSpeed(config->defaultProfile.maxSpeed)) {
                Serial.println("[WebInterface] Warning: Failed to update StepperController maxSpeed");
            } else {
                Serial.println("[WebInterface] Live speed update applied");
            }
        }
        
        if (accelUpdated) {
            if (!StepperController::setAcceleration(config->defaultProfile.acceleration)) {
                Serial.println("[WebInterface] Warning: Failed to update StepperController acceleration");
            } else {
                Serial.println("[WebInterface] Live acceleration update applied");
            }
        }
    } else {
        // Save to flash for permanent updates
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
        // Use centralized validation with automatic clamping
        if (!InputValidation::validateInt32(pos, ParamLimits::MIN_POSITION, 
                                           ParamLimits::MAX_POSITION, "move position")) {
            Serial.println("[WebInterface] Move position out of range, clamped");
        }
        // Update the command with the validated value
        const_cast<JsonDocument&>(cmd)["position"] = pos;
        return true;
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