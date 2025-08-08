// DMX Diagnostic Enhancement for DMXReceiver.cpp
// Add this function to help debug channel mapping issues

/**
 * Debug function to show raw DMX values for troubleshooting
 * Shows the first 20 channels to help identify channel mapping
 */
static void debugRawDMXChannels() {
    Serial.print("[DMX DEBUG] Raw channels 1-20: ");
    for (int i = 1; i <= 20; i++) {
        uint8_t value = dmx.read(i);
        Serial.printf("[%d:%d] ", i, value);
    }
    Serial.println();
    
    // Also show what we think we're reading
    Serial.printf("[DMX DEBUG] Our 5 channels (base=%d): ", baseChannel);
    for (int i = 0; i < NUM_CHANNELS; i++) {
        Serial.printf("[%d:%d] ", baseChannel + i, channelCache[i]);
    }
    Serial.println();
}

// Modified processDMXChannels function with better diagnostics
static void processDMXChannels() {
    if (!dmxEnabled) {
        return;
    }
    
    // Add periodic raw channel debug
    static uint32_t lastRawDebugTime = 0;
    if (millis() - lastRawDebugTime > 2000) {  // Every 2 seconds
        lastRawDebugTime = millis();
        debugRawDMXChannels();  // Show raw DMX values
    }
    
    // ... rest of the function continues as before ...
    
    // When in CONTROL mode, add more detailed debug
    if (currentMode == DMXMode::CONTROL && !homingRequired) {
        
        // Add channel mapping verification
        static uint32_t lastMappingCheckTime = 0;
        if (millis() - lastMappingCheckTime > 5000) {  // Every 5 seconds
            lastMappingCheckTime = millis();
            
            Serial.println("[DMX DEBUG] Channel Mapping Check:");
            Serial.printf("  Base Channel: %d\n", baseChannel);
            Serial.printf("  Position MSB: Channel %d = %d\n", baseChannel + CH_POSITION_MSB, channels[CH_POSITION_MSB]);
            Serial.printf("  Position LSB: Channel %d = %d\n", baseChannel + CH_POSITION_LSB, channels[CH_POSITION_LSB]);
            Serial.printf("  Speed:        Channel %d = %d\n", baseChannel + CH_SPEED, channels[CH_SPEED]);
            Serial.printf("  Acceleration: Channel %d = %d\n", baseChannel + CH_ACCELERATION, channels[CH_ACCELERATION]);
            Serial.printf("  Mode:         Channel %d = %d\n", baseChannel + CH_MODE, channels[CH_MODE]);
            
            // Check if speed/accel are stuck at 0 while position changes
            static uint8_t lastPosMSB = 0;
            if (channels[CH_POSITION_MSB] != lastPosMSB && 
                channels[CH_SPEED] == 0 && 
                channels[CH_ACCELERATION] == 0) {
                Serial.println("[DMX DEBUG] WARNING: Position changing but speed/accel stuck at 0!");
                Serial.println("[DMX DEBUG] This suggests wrong channel mapping!");
            }
            lastPosMSB = channels[CH_POSITION_MSB];
        }
        
        // ... rest of position processing ...
    }
}

// Add a public function to trigger immediate debug output
bool debugDMXChannels() {
    if (!moduleInitialized) {
        Serial.println("[DMX] Module not initialized");
        return false;
    }
    
    Serial.println("\n==== DMX CHANNEL DEBUG ====");
    
    // Show connection status
    Serial.printf("DMX Connected: %s\n", dmxConnected ? "YES" : "NO");
    Serial.printf("Signal State: %s\n", 
        currentState == DMXState::NO_SIGNAL ? "NO_SIGNAL" :
        currentState == DMXState::SIGNAL_PRESENT ? "SIGNAL_PRESENT" :
        currentState == DMXState::TIMEOUT ? "TIMEOUT" : "ERROR");
    
    // Show raw channels
    debugRawDMXChannels();
    
    // Show configuration
    Serial.printf("\nConfiguration:\n");
    Serial.printf("  Base Channel: %d\n", baseChannel);
    Serial.printf("  16-bit Mode: %s\n", use16BitPosition ? "YES" : "NO");
    Serial.printf("  DMX Enabled: %s\n", dmxEnabled ? "YES" : "NO");
    Serial.printf("  Current Mode: %s\n", 
        currentMode == DMXMode::STOP ? "STOP" :
        currentMode == DMXMode::CONTROL ? "CONTROL" : "HOME");
    
    // Show what channels we're expecting
    Serial.printf("\nExpected Channel Layout (base + offset):\n");
    Serial.printf("  Ch %d: Position MSB (current: %d)\n", baseChannel + 0, channelCache[0]);
    Serial.printf("  Ch %d: Position LSB (current: %d)\n", baseChannel + 1, channelCache[1]);
    Serial.printf("  Ch %d: Acceleration (current: %d)\n", baseChannel + 2, channelCache[2]);
    Serial.printf("  Ch %d: Speed (current: %d)\n", baseChannel + 3, channelCache[3]);
    Serial.printf("  Ch %d: Mode (current: %d)\n", baseChannel + 4, channelCache[4]);
    
    Serial.println("==========================\n");
    
    return true;
}

// Temporary fix: Add channel remapping capability
static struct {
    bool useRemapping = false;
    uint8_t speedChannelOffset = 3;      // Default: base + 3
    uint8_t accelChannelOffset = 2;      // Default: base + 2
} g_channelRemap;

// Function to set custom channel mapping
bool setChannelMapping(uint8_t speedOffset, uint8_t accelOffset) {
    if (speedOffset > 10 || accelOffset > 10) {
        Serial.println("[DMX] Invalid channel offsets");
        return false;
    }
    
    g_channelRemap.speedChannelOffset = speedOffset;
    g_channelRemap.accelChannelOffset = accelOffset;
    g_channelRemap.useRemapping = true;
    
    Serial.printf("[DMX] Channel remapping enabled:\n");
    Serial.printf("  Speed: base + %d (channel %d)\n", speedOffset, baseChannel + speedOffset);
    Serial.printf("  Acceleration: base + %d (channel %d)\n", accelOffset, baseChannel + accelOffset);
    
    return true;
}
