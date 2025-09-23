# Debug Information Guide

## Build Configuration
The project is now built in **Debug mode** with full debug symbols (`-g -O0 -DDEBUG`).

## Binary Information
- **Location**: `cpp/build/ZED_SpatialMappingFusion`  
- **Size**: ~3MB (includes debug symbols)
- **Debug Info**: Contains .debug_* sections for full debugging support
- **Status**: Not stripped, with debug_info

## Key Breakpoint Locations for GUI Capture Mode

### Main Entry Points
```cpp
// main.cpp:456 - Command line argument parsing
// main.cpp:105 - GUI device detection (device 47797222)
// main.cpp:119 - GUI capture initialization
```

### CaptureGUI Class (Device 47797222)
```cpp
// CaptureGUI.cpp:71 - GUI initialization
// CaptureGUI.cpp:94 - Camera opening with specific serial number
// CaptureGUI.cpp:125 - Recording start
// CaptureGUI.cpp:215 - Button click handling
// CaptureGUI.cpp:432 - Display callback (render loop)
// CaptureGUI.cpp:283 - Camera frame update
```

### Button Interaction
```cpp
// CaptureGUI.cpp:469 - Mouse callback
// CaptureGUI.cpp:215 - Button click detection
// CaptureGUI.cpp:217 - Finish button (green) - saves recording
// CaptureGUI.cpp:222 - Cancel button (red) - deletes recording
```

### Capture Recording (Standard Mode)
```cpp
// main.cpp:167 - Standard capture recorder creation  
// CaptureRecorder.cpp:24 - Recording start
// CaptureRecorder.cpp:101 - Recording loop
// CaptureRecorder.cpp:72 - Recording stop
```

### Fusion Mode
```cpp
// main.cpp:234 - Fusion initialization
// main.cpp:294 - Camera subscription to fusion
// main.cpp:357 - Fusion processing loop
// main.cpp:369 - Spatial mapping updates
```

## Debug Commands

### GDB Commands
```bash
# Start debugging
gdb ./ZED_SpatialMappingFusion

# Set breakpoints
break main
break CaptureGUI::init
break CaptureGUI::handleButtonClick
break run_capture_mode

# Run with arguments  
run capture 2.json --duration 20
```

### VSCode Debugging
1. **Select Configuration**: "ZED Capture Mode" or "ZED Fusion Mode"
2. **Set Breakpoints**: Click on line numbers in editor
3. **Start Debugging**: Press F5
4. **Debug Controls**:
   - F10: Step over
   - F11: Step into  
   - Shift+F11: Step out
   - F5: Continue

## Library Paths
- **ZED SDK**: `/usr/local/zed/lib`
- **CUDA**: `/usr/local/cuda/lib64`  
- **Environment**: Automatically set in launch.json

## Notes
- GUI mode only activates for device ID **47797222**
- Preview shows **right camera** feed  
- Buttons are in **right half** of screen (50% each)
- All other devices use standard capture mode
