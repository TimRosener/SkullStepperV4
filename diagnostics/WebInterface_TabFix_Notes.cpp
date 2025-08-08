// Fix for the status tabs visibility issue
// The problem is in the WebInterface.cpp file

// Issue found:
// 1. The HTML structure is correct with proper tab buttons and content divs
// 2. The CSS has styling for .status-tabs but might have display issues
// 3. The JavaScript showStatusTab() function is implemented correctly

// The fix needed in getMainCSS() function:
// Make sure the .status-tab and .status-tab.active CSS rules are properly defined

// Here's the corrected CSS section that should be added/verified in getMainCSS():

/*
.status-tabs {
    display: flex;
    gap: 5px;
    margin-bottom: 20px;
    border-bottom: 2px solid var(--border-color);
}

.status-tab {
    display: none;
    animation: fadeIn 0.3s;
}

.status-tab.active {
    display: block;
}
*/

// Additionally, in the HTML, the initial state should have:
// - First tab button with class="tab-btn active"
// - First tab content div with class="status-tab active"

// This is already correctly implemented in the code, so the issue is likely
// that the .status-tab CSS rules are missing or incomplete.

// TO FIX:
// In WebInterface.cpp, in the getMainCSS() function, verify that after the
// .config-tab rules, there are corresponding .status-tab rules:

/*
.status-tab {
    display: none;
    animation: fadeIn 0.3s;
}

.status-tab.active {
    display: block;
}
*/
