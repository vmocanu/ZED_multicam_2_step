# Camera Selection & Reconstruct Mode Features

## 🎯 **New Features Added**

Successfully added comprehensive camera selection and spatial mapping reconstruction capabilities to your ZED multi-camera system.

## 📋 **Camera Selection Feature**

### **Command-Line Syntax:**
```bash
./ZED_SpatialMappingFusion capture <config_file> --camera <selection> [options]
```

### **Camera Options:**
- **`zedx-mini`**: Only use ZED-X-Mini camera (SN: 57709210)
- **`zedx`**: Only use ZED-X camera (SN: 47797222) 
- **`both`**: Use both cameras (default)

### **Examples:**
```bash
# Use only ZED-X-Mini camera
./ZED_SpatialMappingFusion capture 2.json --camera zedx-mini --duration 30

# Use only ZED-X camera with GUI
./ZED_SpatialMappingFusion capture 2.json --camera zedx --depth-mode neural_plus

# Use both cameras (default behavior)
./ZED_SpatialMappingFusion capture 2.json --camera both --resolution 720
```

### **Smart Camera Detection:**
- **Automatic validation**: Shows error if selected camera is not available
- **Clear feedback**: Displays which cameras are active
- **Dynamic GUI**: GUI mode activates automatically for ZED-X (SN: 47797222) when available

## 🔧 **Enhanced Logging System**

### **Single Camera Mode:**
- Same format as before with clean, focused output
- Shows camera name (ZED-X, ZED-X-Mini) in logs

### **Multi-Camera Mode:**
- **Per-camera labeling**: Each log line shows `[ZED-X]` or `[ZED-X-Mini]`
- **Individual timing**: Frame timing and FPS for each camera separately
- **Clear identification**: Easy to distinguish performance per camera

### **Example Output:**
```
[ZED-X] Frame 1: 0 ms (first frame)
[ZED-X-Mini] Frame 1: 0 ms (first frame)
[ZED-X] Frame 2: 33 ms
[ZED-X-Mini] Frame 2: 35 ms
[ZED-X] Recording... Frames: 150, Duration: 5.0s, FPS: 30.0
[ZED-X-Mini] Recording... Frames: 145, Duration: 5.0s, FPS: 29.0
```

## 🏗️ **Reconstruct Mode**

### **Purpose:**
Perform spatial mapping and 3D reconstruction on recorded SVO files to generate high-quality mesh files.

### **Command-Line Syntax:**
```bash
./ZED_SpatialMappingFusion reconstruct <svo_file> [options]
```

### **Options:**
- **`--resolution <res>`**: Processing resolution: 720 or 1080 (default: 1080)

### **Examples:**
```bash
# Basic reconstruction
./ZED_SpatialMappingFusion reconstruct camera_47797222.svo

# High-performance reconstruction at 720p
./ZED_SpatialMappingFusion reconstruct camera_57709210.svo --resolution 720

# Maximum quality reconstruction at 1080p
./ZED_SpatialMappingFusion reconstruct recording.svo --resolution 1080
```

### **Reconstruction Process:**
1. **📂 File Validation**: Checks if SVO file exists
2. **🔧 Camera Initialization**: Opens SVO with optimal settings (NEURAL_PLUS depth)
3. **🗺️ Spatial Mapping Setup**: 5cm resolution, 2GB memory, texture enabled
4. **⚡ Frame Processing**: Processes all frames with progress indicators
5. **🎯 Mesh Extraction**: Generates complete 3D mesh
6. **💾 File Output**: Saves as `filename_mesh.ply`

### **Output:**
- **Mesh file**: `<original_name>_mesh.ply`
- **Statistics**: Vertex count, triangle count
- **Progress**: Real-time FPS and completion percentage

## 🎯 **Performance Benefits**

### **Camera Selection Advantages:**
- **🚀 Focused Performance**: Use single camera for maximum FPS
- **🔧 Testing Flexibility**: Test each camera individually
- **📊 Detailed Monitoring**: Per-camera performance metrics
- **⚡ Reduced Resource Usage**: Lower CPU/memory with single camera

### **Reconstruct Mode Benefits:**
- **🏗️ Offline Processing**: Process recordings when convenient
- **🎯 Optimal Quality**: Uses NEURAL_PLUS for best spatial mapping
- **📈 Progress Tracking**: Clear progress indicators and timing
- **💾 Portable Output**: Standard PLY format for 3D software

## 📊 **Performance Recommendations**

### **For Your ZED Box Orin 16GB:**

#### **Single Camera Testing:**
```bash
# Test ZED-X performance
./ZED_SpatialMappingFusion capture 2.json --camera zedx --resolution 720 --depth-mode neural_light

# Test ZED-X-Mini performance  
./ZED_SpatialMappingFusion capture 2.json --camera zedx-mini --resolution 720 --depth-mode neural_light
```
**Expected**: 25-30fps per camera, easy troubleshooting

#### **Production Dual Camera:**
```bash
# Both cameras with optimized settings
./ZED_SpatialMappingFusion capture 2.json --camera both --resolution 720 --depth-mode neural
```
**Expected**: 20-25fps total, labeled logs for each camera

#### **High-Quality Reconstruction:**
```bash
# After capturing, reconstruct for spatial mapping
./ZED_SpatialMappingFusion reconstruct camera_47797222.svo --resolution 1080
```
**Expected**: High-quality 3D mesh suitable for spatial mapping applications

## 🛠️ **Technical Implementation**

### **Camera Filtering:**
- **Serial Number Matching**: Filters cameras by exact SN (57709210, 47797222)
- **Configuration Validation**: Shows available cameras if selection fails
- **Smart GUI Activation**: Auto-detects GUI-capable camera (47797222)

### **Enhanced CaptureRecorder:**
- **Camera Naming**: `setCameraName()` method for labeled logging
- **Per-Instance Logging**: Each recorder logs with camera identifier
- **Thread-Safe Output**: Clean, non-overlapping log output

### **Spatial Mapping Reconstruction:**
- **Optimal Parameters**: 5cm resolution, texture enabled
- **Progress Monitoring**: Real-time FPS and percentage completion
- **Error Handling**: Comprehensive error checking and reporting
- **Standard Output**: PLY format compatible with Blender, MeshLab, etc.

## ✅ **Validation & Error Handling**

### **Camera Selection:**
- ✅ **Invalid camera names** → Falls back to 'both' with warning
- ✅ **Missing cameras** → Shows available cameras and exits gracefully
- ✅ **Serial number matching** → Exact matching with clear feedback

### **Reconstruct Mode:**
- ✅ **File validation** → Checks SVO file exists before processing
- ✅ **SDK error handling** → Clear error messages for all ZED operations
- ✅ **Progress indication** → Real-time feedback during long operations
- ✅ **Output validation** → Confirms successful mesh generation and saving

## 🎯 **Usage Workflow**

### **1. Test Individual Cameras:**
```bash
# Test each camera separately
./ZED_SpatialMappingFusion capture 2.json --camera zedx-mini --duration 10
./ZED_SpatialMappingFusion capture 2.json --camera zedx --duration 10
```

### **2. Production Recording:**
```bash
# Record with both cameras
./ZED_SpatialMappingFusion capture 2.json --camera both --duration 60
```

### **3. Spatial Mapping:**
```bash
# Generate 3D meshes from recordings
./ZED_SpatialMappingFusion reconstruct camera_57709210.svo
./ZED_SpatialMappingFusion reconstruct camera_47797222.svo
```

### **4. Analysis:**
- Review per-camera logs for performance tuning
- Use generated PLY files for spatial mapping applications
- Adjust parameters based on individual camera performance

Your ZED multi-camera system now provides precise control over camera usage, detailed performance monitoring, and professional spatial mapping reconstruction capabilities! 🎯
