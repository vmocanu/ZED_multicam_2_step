# Frame Logging Implementation

## Overview
Added comprehensive frame logging functionality that displays frame ID and time delta (in milliseconds) since the last frame was captured.

## Implementation Details

### 🎯 **Both Capture Modes Support Frame Logging:**
1. **GUI Capture Mode** (device 47797222) - `CaptureGUI` class
2. **Standard Capture Mode** (all other devices) - `CaptureRecorder` class

### 📊 **Logging Format:**
```
Frame 1: 0 ms (first frame)
Frame 2: 33 ms
Frame 3: 34 ms
Frame 4: 33 ms
...
```

### 🔧 **Technical Implementation:**

#### **CaptureGUI.hpp** - Added Variables:
```cpp
// Frame timing for logging
std::chrono::steady_clock::time_point last_frame_time;
bool first_frame;
```

#### **CaptureGUI.cpp** - Constructor Update:
```cpp
CaptureGUI::CaptureGUI() 
    : /* existing initialization */ , first_frame(true) {
```

#### **CaptureGUI.cpp** - updateCamera() Method:
```cpp
if (recording) {
    frame_count++;
    
    // Log frame timing information
    auto current_time = std::chrono::steady_clock::now();
    if (first_frame) {
        std::cout << "Frame " << frame_count << ": 0 ms (first frame)" << std::endl;
        first_frame = false;
    } else {
        auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_frame_time).count();
        std::cout << "Frame " << frame_count << ": " << time_since_last << " ms" << std::endl;
    }
    last_frame_time = current_time;
}
```

#### **CaptureRecorder.hpp** - Added Variables:
```cpp
// Frame timing for logging  
std::chrono::steady_clock::time_point last_frame_time;
bool first_frame;
```

#### **CaptureRecorder.cpp** - Constructor Updates:
```cpp
CaptureRecorder::CaptureRecorder() : recording(false), frame_count(0), first_frame(true) {

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode) 
    : recording(false), frame_count(0), first_frame(true) {
```

#### **CaptureRecorder.cpp** - recordingLoop() Method:
```cpp
if (grab_status == sl::ERROR_CODE::SUCCESS) {
    frame_count++;
    
    // Log frame timing information
    auto now = std::chrono::steady_clock::now();
    if (first_frame) {
        std::cout << "Frame " << frame_count << ": 0 ms (first frame)" << std::endl;
        first_frame = false;
    } else {
        auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_frame_time).count();
        std::cout << "Frame " << frame_count << ": " << time_since_last << " ms" << std::endl;
    }
    last_frame_time = now;
    
    // Print progress every 5 seconds
    // [existing progress logging continues...]
}
```

## 📈 **Expected Output Example:**

### GUI Capture Mode (Device 47797222):
```
=== CAPTURE MODE ===
Found special device 47797222 - using GUI capture mode
Started recording to: ./svo_recordings/camera_47797222.svo
Frame 1: 0 ms (first frame)
Frame 2: 33 ms
Frame 3: 34 ms
Frame 4: 33 ms
Frame 5: 33 ms
...
```

### Standard Capture Mode (Other Devices):
```
=== CAPTURE MODE ===
Using standard capture mode
Starting recording for camera 12345...
Frame 1: 0 ms (first frame)
Frame 2: 33 ms
Frame 3: 34 ms
Frame 4: 33 ms
Recording... Frames: 150, Duration: 5.0s
...
```

## 🏃 **Performance Notes:**
- **Minimal Overhead**: Timing calculations only occur when frames are successfully captured
- **High Precision**: Uses `std::chrono::steady_clock` for microsecond accuracy, displayed in milliseconds
- **Real-time Display**: Frame timing appears immediately in console output
- **Typical Frame Rate**: ~30 FPS (33ms intervals) for HD1080 @ 30fps configuration

## 🎯 **Use Cases:**
1. **Performance Monitoring**: Track frame rate consistency
2. **Debugging**: Identify frame drops or timing issues  
3. **Optimization**: Measure capture performance improvements
4. **Analysis**: Correlate timing with camera behavior

## ✅ **Build Status:**
- **Compilation**: ✅ Success (Debug build with full symbols)
- **Linting**: ✅ No errors
- **Integration**: ✅ Works with both GUI and standard modes
- **Backward Compatibility**: ✅ Maintained
