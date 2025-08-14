// Fix for the status tabs visibility issue - RESOLVED in v4.1.12
// The problem was in the WebInterface.cpp file

// Issue found:
// The JavaScript showStatusTab() function was using event.target without 
// having the event parameter passed from the onclick handlers.

// Fix applied:
// 1. Updated onclick handlers to pass event: onclick="showStatusTab('system', event)"
// 2. Updated function signature: function showStatusTab(tabName, event)
// 3. Added safety check: if (event && event.target)

// VERIFIED: The CSS was already correct with proper .status-tab rules.

// ============================================================================
// System Diagnostics Tab Implementation - Added in v4.1.13
// ============================================================================

// New features added:
// 1. Third tab "Diagnostics" in the status section
// 2. Memory status display:
//    - Free heap with percentage and color coding
//    - Total heap size
//    - Minimum free heap since boot
//    - Largest allocatable block (fragmentation indicator)
// 
// 3. Task health monitoring:
//    - StepperCtrl [Core 0] - shows running status
//    - DMXReceiver [Core 0] - shows running status
//    - WebServer [Core 1] - shows running status with stack info
//    - Broadcast [Core 1] - shows running status with stack info
//
// 4. System information:
//    - CPU Model (ESP32-S3)
//    - CPU Frequency
//    - Flash Size
//    - Reset Reason (Power On, Software Reset, etc.)
//
// Implementation notes:
// - Task handles for StepperController and DMXReceiver are not directly accessible
// - Used proxy indicators (motion state, DMX state) to determine if tasks are running
// - WebInterface tasks have direct access to handles for stack monitoring
// - Memory percentage color coding: <20% red, <40% yellow, >=40% green
