/*
 * WebInterface.h
 * 
 * WiFi Access Point and Web-based control interface for SkullStepperV4
 * Provides wireless monitoring and configuration capabilities
 * 
 * Part of SkullStepperV4 - Phase 7
 * 
 * REQUIRED LIBRARIES:
 * 1. ESPAsyncWebServer by me-no-dev
 * 2. AsyncTCP by me-no-dev (REQUIRED DEPENDENCY)
 * 
 * Install via Arduino Library Manager:
 * - Tools -> Manage Libraries
 * - Search for "ESPAsyncWebServer" and install
 * - Search for "AsyncTCP" and install
 * 
 * Architecture:
 * - Runs on Core 1 (Communication core)
 * - Completely isolated from other modules
 * - Communicates via GlobalInfrastructure queues and protected data
 * - Non-blocking async operation
 */

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "ProjectConfig.h"  // Get feature switches
#include "GlobalInterface.h"
#include <Arduino.h>  // Need this for String type

#ifdef ENABLE_WEB_INTERFACE  // Only include web server dependencies if enabled

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>

// Web server configuration
#define WEB_SERVER_PORT 80
#define WS_MAX_CLIENTS 2
#define WS_BUFFER_SIZE 1024
#define STATUS_BROADCAST_INTERVAL_MS 100  // 10Hz updates
#define JSON_BUFFER_SIZE 2048

// WiFi Access Point defaults
#define DEFAULT_AP_SSID "SkullStepper"
#define DEFAULT_AP_PASSWORD ""  // Empty = open network
#define DEFAULT_AP_CHANNEL 6
#define AP_MAX_CONNECTIONS 4

// Task configuration
#define WEB_SERVER_TASK_STACK_SIZE 8192
#define BROADCAST_TASK_STACK_SIZE 4096
#define WEB_SERVER_TASK_PRIORITY 1
#define BROADCAST_TASK_PRIORITY 1

class WebInterface {
public:
    // Singleton pattern
    static WebInterface& getInstance();
    
    // Module control
    void begin();
    void update();  // Called from main loop if needed
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
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    
    // State
    bool enabled;
    bool running;
    volatile uint8_t activeClients;
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
    void setupRoutes();
    void setupWebSocket();
    
    // HTTP handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleStatus(AsyncWebServerRequest* request);
    void handleCommand(AsyncWebServerRequest* request);
    void handleConfig(AsyncWebServerRequest* request);
    void handleConfigUpdate(AsyncWebServerRequest* request);
    void handleInfo(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    
    // WebSocket handlers
    void onWsEvent(AsyncWebSocket* server, 
                   AsyncWebSocketClient* client,
                   AwsEventType type,
                   void* arg,
                   uint8_t* data,
                   size_t len);
    void handleWsMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);
    
    // Internal methods
    void getSystemStatus(JsonDocument& doc);
    void getSystemConfig(JsonDocument& doc);
    void getSystemInfo(JsonDocument& doc);
    bool sendMotionCommand(CommandType type, int32_t position = 0);
    bool updateConfiguration(const JsonDocument& params);
    void broadcastStatus();
    void sendStatusToClient(AsyncWebSocketClient* client);
    bool validateCommand(const JsonDocument& cmd);
    
    // HTML content (stored in PROGMEM)
    static const char index_html[] PROGMEM;
    static const char style_css[] PROGMEM;
    static const char script_js[] PROGMEM;
};

// HTML/CSS/JS content will be defined in the .cpp file
// Using PROGMEM to store in flash instead of RAM

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