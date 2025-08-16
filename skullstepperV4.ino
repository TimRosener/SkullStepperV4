// ============================================================================
// File: skullstepperV4.ino - Thread-Safe Main Sketch with StepperController
// Project: SkullStepperV4 - ESP32-S3 Modular Stepper Control System
// Version: 4.1.14
// Date: 2025-02-08
// Author: Tim Rosener
// Description: Main Arduino sketch with complete system integration
// License: MIT
// 
// IMPORTANT: This sketch runs on Core 1 (Arduino default) for monitoring/communication
//           Real-time modules (StepperController, DMX, Safety) run on Core 0
// ============================================================================

#include "GlobalInterface.h"
#include "StepperController.h"
#include "SerialInterface.h"
#include "SystemConfig.h"
#include "ProjectConfig.h"  // Global project configuration
#include <esp_task_wdt.h>   // ESP32 Task Watchdog Timer

// Optional modules are now configured in ProjectConfig.h

#ifdef ENABLE_WEB_INTERFACE
#include "WebInterface.h"
#endif

#include "DMXReceiver.h"  // DMX512 input module

// Forward declaration of global infrastructure function
bool initializeGlobalInfrastructure();
void printInfrastructureStatus();
bool validateSystemIntegrity();

// ============================================================================
// CRITICAL: Thread-Safe Initialization Order
// 1. Global Infrastructure (mutexes, queues, data structures)
// 2. SystemConfig (ESP32 flash storage with thread safety)
// 3. SerialInterface (complete command system with JSON API)
// 4. StepperController (ODStepper motion control with thread safety)
// 5. Other modules (when ready)
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000); // Give serial time to initialize
  Serial.println();
  Serial.println("============================================================================");
  Serial.println("SkullStepperV4 - ESP32-S3 Thread-Safe Stepper Control");
  Serial.println("Version: 4.1.14 - Fixed Limit Safety Margin & Web Interface");
  Serial.println("Memory-Safe, Thread-Safe Architecture");
  Serial.println("============================================================================");
  
  // ========================================================================
  // Initialize Watchdog Timer for System Safety
  // ========================================================================
  Serial.println("Initializing watchdog timer (10 second timeout)...");
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 10000,       // 10 seconds timeout
    .idle_core_mask = 0,       // Not monitoring idle tasks
    .trigger_panic = true      // Panic (reset) on timeout
  };
  esp_task_wdt_init(&wdt_config);  // Initialize watchdog with config
  esp_task_wdt_add(NULL);           // Add current task (setup/loop) to watchdog
  Serial.println("✓ Watchdog timer active");
  
  // ========================================================================
  // STEP 1: Initialize Thread-Safe Global Infrastructure
  // ========================================================================
  Serial.println("STEP 1: Initializing thread-safe global infrastructure...");
  if (!initializeGlobalInfrastructure()) {
    Serial.println("FATAL: Global infrastructure initialization failed");
    Serial.println("System cannot continue without thread safety");
    while (true) {
      delay(1000);
      Serial.println("System halted - infrastructure init failed");
    }
  }
  
  Serial.println("✓ Thread-safe infrastructure ready");
  Serial.println("✓ FreeRTOS mutexes and queues created");
  Serial.println("✓ Memory-safe data structures initialized");
  
  // ========================================================================
  // STEP 2: Initialize SystemConfig with ESP32 Flash Storage
  // ========================================================================
  Serial.println("\nSTEP 2: Initializing configuration management...");
  if (!SystemConfigMgr::initialize()) {
    Serial.println("FATAL: SystemConfig initialization failed");
    Serial.println("Configuration will not persist across reboots");
    while (true) {
      delay(1000);
      Serial.println("System halted - config init failed");
    }
  }
  
  Serial.println("✓ ESP32 flash storage ready");
  Serial.println("✓ Configuration persistence enabled");
  Serial.println("✓ Thread-safe parameter management active");
  
  // ========================================================================
  // STEP 3: Initialize SerialInterface with Complete Command System
  // ========================================================================
  Serial.println("\nSTEP 3: Initializing serial interface...");
  if (!SerialInterface::initialize()) {
    Serial.println("FATAL: SerialInterface initialization failed");
    Serial.println("Command processing will not be available");
    while (true) {
      delay(1000);
      Serial.println("System halted - serial interface init failed");
    }
  }
  
  Serial.println("✓ Complete command system ready");
  Serial.println("✓ JSON API enabled");
  Serial.println("✓ Configuration management commands active");
  Serial.println("✓ Interactive help system ready");
  
  // ========================================================================
  // STEP 4: Initialize StepperController with ODStepper
  // ========================================================================
  Serial.println("\nSTEP 4: Initializing stepper controller...");
  if (!StepperController::initialize()) {
    Serial.println("FATAL: Stepper controller initialization failed");
    Serial.println("Motion control will not be available");
    while (true) {
      delay(1000);
      Serial.println("System halted - stepper controller init failed");
    }
  }
  
  Serial.println("✓ ODStepper motion control ready");
  Serial.println("✓ Core 0 real-time task running");
  Serial.println("✓ Limit switches configured");
  Serial.println("✓ CL57Y ALARM monitoring active");
  Serial.println("✓ Thread-safe motion command queue ready");
  
  // ========================================================================
  // STEP 5: Initialize DMXReceiver (Phase 6 Development)
  // ========================================================================
  Serial.println("\nSTEP 5: Initializing DMX receiver...");
  if (!DMXReceiver::initialize()) {
    Serial.println("WARNING: DMX receiver initialization failed");
    Serial.println("DMX control will not be available");
  } else {
    Serial.println("✓ ESP32S3DMX library initialized");
    Serial.println("✓ Core 0 DMX task running");
    Serial.println("✓ DMX signal monitoring active");
    Serial.println("✓ 5-channel configuration ready");
  }
  
  // ========================================================================
  // STEP 6: Initialize WebInterface (Optional)
  // ========================================================================
  #ifdef ENABLE_WEB_INTERFACE
  Serial.println("\nSTEP 5: Initializing web interface...");
  WebInterface::getInstance().begin();
  Serial.println("✓ WiFi Access Point created");
  Serial.print("✓ Web interface available at: http://");
  Serial.println(WebInterface::getInstance().getAPAddress());
  Serial.println("✓ Connect to WiFi: SkullStepper (open network)");
  #endif
  
  // ========================================================================
  // STEP 7: Validate System Integrity
  // ========================================================================
  Serial.println("\nSTEP 6: Validating system integrity...");
  if (!validateSystemIntegrity()) {
    Serial.println("WARNING: System integrity validation failed");
    Serial.println("Continuing with reduced functionality");
  } else {
    Serial.println("✓ System integrity validation passed");
  }
  
  // ========================================================================
  // System Ready
  // ========================================================================
  setSystemState(SystemState::READY);
  
  Serial.println("============================================================================");
  Serial.println("COMPLETE THREAD-SAFE MOTION CONTROL SYSTEM READY");
  Serial.println("DMX RECEIVER MODULE ACTIVE (Phase 1 Complete)");
  Serial.println("============================================================================");
  Serial.println("Features enabled:");
  Serial.println("  ✓ Thread-safe operation (FreeRTOS mutexes & queues)");
  Serial.println("  ✓ Memory-safe data access (protected structures)");
  Serial.println("  ✓ ESP32 flash configuration storage (persistent settings)");
  Serial.println("  ✓ Complete interactive command system");
  Serial.println("  ✓ JSON API for automation");
  Serial.println("  ✓ Real-time parameter configuration");
  Serial.println("  ✓ ODStepper motion control (trapezoidal profiles)");
  Serial.println("  ✓ Auto-range homing sequence");
  Serial.println("  ✓ Hardware limit switch protection");
  Serial.println("  ✓ CL57Y ALARM monitoring");
  #ifdef ENABLE_WEB_INTERFACE
  Serial.println("  ✓ Web interface on WiFi AP (http://192.168.4.1)");
  #endif
  Serial.println("============================================================================");
  Serial.println("FULL COMMAND SYSTEM AVAILABLE:");
  Serial.println("Motion: MOVE <pos>, MOVEHOME, HOME, STOP, ESTOP, ENABLE, DISABLE");
  Serial.println("Config: CONFIG, CONFIG SET <param> <value>, CONFIG RESET [param]");
  Serial.println("Info: STATUS, PARAMS, HELP");
  Serial.println("Interface: ECHO ON/OFF, VERBOSE 0-3, JSON ON/OFF, STREAM ON/OFF");
  Serial.println("Testing: TEST (stress test), TEST2/RANDOMTEST (random positions)");
  Serial.println("DMX: DMX STATUS, DMX MONITOR (for testing)");
  Serial.println("System: Infrastructure test commands available");
  Serial.println("============================================================================");
  Serial.println("Type 'HELP' for complete command reference");
  Serial.println("Type 'STATUS' for system overview");
  Serial.println("Type 'CONFIG' to see all settings");
  Serial.println();
  
  // ========================================================================
  // Auto-Home on Boot (if enabled)
  // ========================================================================
  SystemConfig* config = SystemConfigMgr::getConfig();
  if (config && config->autoHomeOnBoot) {
    Serial.println("============================================================================");
    Serial.println("AUTO-HOME ON BOOT ENABLED - Starting automatic homing sequence...");
    Serial.println("============================================================================");
    
    // Queue home command
    if (StepperController::startHoming()) {
      Serial.println("✓ Homing sequence started");
      Serial.println("Please wait for homing to complete...");
      
      // Wait for homing to complete with progress updates
      uint32_t homingStartTime = millis();
      uint8_t lastProgress = 0;
      
      while (StepperController::isHoming()) {
        uint8_t progress = StepperController::getHomingProgress();
        if (progress != lastProgress) {
          Serial.printf("Homing progress: %d%%\n", progress);
          lastProgress = progress;
        }
        delay(100);
        
        // Timeout after 120 seconds
        if (millis() - homingStartTime > 120000) {
          Serial.println("WARNING: Homing timeout - manual homing may be required");
          break;
        }
      }
      
      if (StepperController::isHomed()) {
        Serial.println("✓ Auto-homing completed successfully!");
        Serial.println("System is ready for motion commands.");
      } else {
        Serial.println("✗ Auto-homing failed - manual homing required");
      }
    } else {
      Serial.println("✗ Failed to start auto-homing sequence");
    }
    Serial.println();
  } else {
    Serial.println("*** IMPORTANT: You must HOME the system before any movement! ***");
    Serial.println("1. Type 'HOME' to run auto-range detection");
    Serial.println("2. Wait for homing to complete");
    Serial.println("3. Then use movement commands like 'MOVE 100'");
    Serial.println();
    Serial.println("Movement is blocked until homing establishes safe position limits.");
    Serial.println();
  }
}

void loop() {
  // ========================================================================
  // Core 1 Loop - Communication and Monitoring
  // ========================================================================
  
  // Note: StepperController runs on Core 0 in its own task
  // This loop handles Core 1 responsibilities only
  
  // Feed watchdog timer periodically
  static uint32_t lastWdtFeed = 0;
  if (millis() - lastWdtFeed > 3000) {  // Feed every 3 seconds
    esp_task_wdt_reset();
    lastWdtFeed = millis();
    
    // Check if critical tasks are healthy
    if (!StepperController::isTaskHealthy()) {
      Serial.println("CRITICAL: StepperController task not responding!");
      Serial.println("System will restart in 7 seconds unless task recovers...");
    }
  }
  
  // Update serial interface (COMPLETE command processing system with JSON API)
  // This handles ALL commands: human-readable, JSON API, skull> prompt, etc.
  SerialInterface::update();
  
  // Update web interface if enabled
  #ifdef ENABLE_WEB_INTERFACE
  WebInterface::getInstance().update();
  #endif
  
  // Update system uptime in thread-safe status
  updateSystemUptime();
  
  // ========================================================================
  // Periodic System Maintenance (Thread-Safe)
  // ========================================================================
  
  static uint32_t lastStatusBroadcast = 0;
  static uint32_t lastStepperCheck = 0;
  uint32_t currentTime = millis();
  
  // Broadcast status updates every 5 seconds (thread-safe)
  if (currentTime - lastStatusBroadcast >= 5000) {
    broadcastStatusUpdate();
    lastStatusBroadcast = currentTime;
  }
  
  // Check stepper status every 100ms
  if (currentTime - lastStepperCheck >= 100) {
    // Update motion-related status info
    if (StepperController::isHoming()) {
      Serial.printf("Homing progress: %d%%\n", StepperController::getHomingProgress());
    }
    
    lastStepperCheck = currentTime;
  }
  
  // Small delay to prevent overwhelming the system
  delay(1);
}

// ============================================================================
// COMPLETE SYSTEM INTEGRATION TEST PROCEDURE:
// 
// 1. Upload this code with ALL modules (GlobalInfrastructure.cpp, 
//    SerialInterface.cpp, SystemConfig.cpp, StepperController.cpp)
// 2. Open Serial Monitor (115200 baud)
// 3. Verify complete initialization sequence
// 4. Test infrastructure: Type 'I' and 'T'
// 5. Test configuration: Type 'CONFIG' to see all settings
// 6. Test homing: Type 'HOME' to run auto-range detection
// 7. Test motion: Type 'ENABLE' then 'MOVE 100'
// 8. Test configuration changes: 'CONFIG SET maxSpeed 2000'
// 9. Test JSON API: {"command":"status"}
// 10. Test help system: Type 'HELP'
// 11. Test limit switches: Manually trigger limits during motion
// 12. Test persistence: Change a setting, restart, verify it's saved
//
// Expected behavior:
// - Auto-range homing detects physical limits
// - Smooth trapezoidal motion profiles via ODStepper
// - Dynamic target updates for future DMX control
// - Hardware limit switch protection (immediate stop)
// - All settings persist across reboots in ESP32 flash
// - Complete interactive command system
// - Thread-safe operation (no crashes or corruption)
// - Memory-safe data access (validated integrity)
// - JSON API for automation integration
// 
// Phase 4 StepperController with ODStepper complete!
// ============================================================================