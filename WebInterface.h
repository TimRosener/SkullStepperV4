/*
 * WebInterface.h
 * 
 * WiFi Access Point and Web-based control interface for SkullStepperV4
 * Using built-in ESP32 WebServer and WebSocketsServer
 * Compatible with ESP32 Arduino Core 3.x
 * 
 * REQUIRED LIBRARY:
 * - WebSockets by Markus Sattler (Install via Arduino Library Manager)
 * 
 * Architecture:
 * - Runs on Core 1 (Communication core)
 * - Completely isolated from other modules
 * - Communicates via GlobalInfrastructure queues and protected data
 * - All content served dynamically - no filesystem needed
 */

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "ProjectConfig.h"
#include "GlobalInterface.h"
#include <Arduino.h>

#ifdef ENABLE_WEB_INTERFACE

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>

// Web server configuration
#define WEB_SERVER_PORT 80
#define WS_SERVER_PORT 81
#define WS_MAX_CLIENTS 2
#define STATUS_BROADCAST_INTERVAL_MS 100  // 10Hz updates
#define JSON_BUFFER_SIZE 2048

// WiFi Access Point defaults
#define DEFAULT_AP_SSID "SkullStepper"
#define DEFAULT_AP_PASSWORD ""  // Empty = open network
#define DEFAULT_AP_CHANNEL 6
#define AP_MAX_CONNECTIONS 4

// Task configuration
#define WEB_TASK_STACK_SIZE 8192
#define BROADCAST_TASK_STACK_SIZE 4096
#define WEB_TASK_PRIORITY 1
#define BROADCAST_TASK_PRIORITY 1

class WebInterface {
public:
    // Singleton pattern
    static WebInterface& getInstance();
    
    // Module control
    void begin();
    void update();  // Called from main loop
    void stop();
    
    // Status
    bool isEnabled() const { return enabled; }
    bool isRunning() const { return running; }
    uint8_t getClientCount() const { return activeClients; }
    String getAPAddress() const;
    
    // Configuration
    void setCredentials(const char* ssid, const char* password);
    void setEnabled(bool enable);
    
private:
    // Singleton constructor
    WebInterface();
    ~WebInterface();
    
    // Prevent copying
    WebInterface(const WebInterface&) = delete;
    WebInterface& operator=(const WebInterface&) = delete;
    
    // Core components
    WebServer* httpServer;
    WebSocketsServer* wsServer;
    DNSServer* dnsServer;
    
    // Client tracking
    volatile uint8_t activeClients;
    SemaphoreHandle_t clientMutex;
    bool clientConnected[WS_MAX_CLIENTS];
    
    // State
    bool enabled;
    bool running;
    uint16_t nextCommandId;
    
    // WiFi configuration
    String apSSID;
    String apPassword;
    uint8_t apChannel;
    IPAddress apIP;
    IPAddress apGateway;
    IPAddress apSubnet;
    
    // Tasks
    TaskHandle_t webTaskHandle;
    TaskHandle_t broadcastTaskHandle;
    
    // Task functions (static for FreeRTOS)
    static void webServerTask(void* parameter);
    static void statusBroadcastTask(void* parameter);
    
    // Setup functions
    void setupWiFi();
    void setupWebServer();
    void setupWebSocket();
    
    // HTTP handlers
    void handleRoot();
    void handleStatus();
    void handleCommand();
    void handleConfig();
    void handleConfigUpdate();
    void handleInfo();
    void handleNotFound();
    void handleCaptivePortal();
    void handleFavicon();
    
    // WebSocket handlers
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    void processWebSocketMessage(uint8_t num, uint8_t* payload, size_t length);
    
    // Internal methods
    void getSystemStatus(JsonDocument& doc);
    void getSystemConfig(JsonDocument& doc);
    void getSystemInfo(JsonDocument& doc);
    bool sendMotionCommand(CommandType type, int32_t position = 0);
    bool updateConfiguration(const JsonDocument& params);
    void broadcastStatus();
    void sendStatusToClient(uint8_t num);
    bool validateCommand(const JsonDocument& cmd);
    String buildJsonResponse(const char* status, const char* message);
    
    // Test update methods
    void updateStressTest();
    void updateRandomTest();
    
    // HTML/CSS/JS content methods
    String getIndexHTML();
    String getMainCSS();
    String getMainJS();
    
    // Response helpers
    void sendJsonResponse(int code, const JsonDocument& doc);
    void sendJsonResponse(int code, const char* status, const char* message);
};

#else // ENABLE_WEB_INTERFACE not defined

// Stub class when WebInterface is disabled
class WebInterface {
public:
    static WebInterface& getInstance() {
        static WebInterface instance;
        return instance;
    }
    void begin() {}
    void update() {}
    void stop() {}
    bool isEnabled() const { return false; }
    bool isRunning() const { return false; }
    uint8_t getClientCount() const { return 0; }
    String getAPAddress() const { return "Disabled"; }
    void setCredentials(const char* ssid, const char* password) {}
    void setEnabled(bool enable) {}
};

#endif // ENABLE_WEB_INTERFACE

#endif // WEBINTERFACE_H