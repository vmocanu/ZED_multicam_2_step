# ZED Spatial Mapping: Separate Capture and Fusion Guide

## Overview

The ZED Spatial Mapping Fusion application now supports **separate capture and fusion workflows**, allowing you to:

1. **Capture Mode**: Record HD1080 video data from multiple ZED cameras into separate SVO files
2. **Fusion Mode**: Process the recorded SVO files to perform spatial mapping fusion
3. **Live Fusion Mode**: Original functionality - live capture + fusion in real-time

## 🎬 **Capture Mode**

### Purpose
Record synchronized data from multiple ZED cameras into individual SVO files for later processing.

### Usage
```bash
./ZED_SpatialMappingFusion capture <config_file> [options]
```

### Options
- `--duration <seconds>`: Recording duration (default: 30 seconds)
- `--output-dir <path>`: Output directory for SVO files (default: ./svo_recordings)

### Examples
```bash
# Record for 60 seconds with default output directory
./ZED_SpatialMappingFusion capture config.json --duration 60

# Record for 30 seconds to custom directory
./ZED_SpatialMappingFusion capture config.json --duration 30 --output-dir /path/to/recordings

# Record with default settings (30 seconds)
./ZED_SpatialMappingFusion capture config.json
```

### Capture Process
1. **Initialize cameras** with HD1080 resolution, 30fps, NEURAL_PLUS depth mode
2. **Create output directory** automatically if it doesn't exist
3. **Start recording** all configured cameras simultaneously
4. **Progress monitoring** with real-time status updates
5. **Graceful shutdown** with Ctrl+C support
6. **Recording summary** with file sizes and locations

### Output Files
SVO files are saved with naming convention: `camera_<serial_number>.svo`

Example output:
```
svo_recordings/
├── camera_12345678.svo   (Camera 1 recording)
├── camera_87654321.svo   (Camera 2 recording)
└── camera_11223344.svo   (Camera 3 recording)
```

## 🔬 **Fusion Mode**

### Purpose
Process previously recorded SVO files to generate spatial mapping fusion results.

### Usage
```bash
./ZED_SpatialMappingFusion fusion <config_file>
```

### Process
1. **Locate SVO files** in `./svo_recordings/` directory
2. **Load configuration** and map SVO files to camera serial numbers
3. **Synchronize SVO playback** across multiple cameras
4. **Perform fusion processing** with spatial mapping
5. **Generate 3D mesh** with real-time visualization
6. **Save results** as PLY mesh file

### Requirements
- SVO files must exist in `./svo_recordings/` directory
- SVO files must match camera serial numbers in configuration
- All cameras in configuration should have corresponding SVO files

### Example
```bash
# First, record data
./ZED_SpatialMappingFusion capture config.json --duration 120

# Then, process the recorded data
./ZED_SpatialMappingFusion fusion config.json
```

## 🔴 **Live Fusion Mode**

### Purpose
Original functionality - live capture and fusion processing in real-time.

### Usage
```bash
./ZED_SpatialMappingFusion live <config_file>
```

This mode performs real-time capture and fusion without saving intermediate SVO files.

## 🚀 **VSCode Integration**

### Debug Configurations Available

#### RelWithDebInfo (Debug + Performance)
- **Debug ZED Live Fusion (RelWithDebInfo)**: Debug live mode
- **Debug ZED Capture Mode (RelWithDebInfo)**: Debug capture mode  
- **Debug ZED Fusion Mode (RelWithDebInfo)**: Debug fusion mode

#### Release (Maximum Performance)
- **Run ZED Live Fusion (Release - Optimized)**: Fast live mode
- **Run ZED Capture Mode (Release - Optimized)**: Fast capture mode
- **Run ZED Fusion Mode (Release - Optimized)**: Fast fusion mode

### Quick Start with VSCode
1. Press **F5**
2. Select the desired configuration
3. The application builds automatically and runs

## 📋 **Workflow Examples**

### Complete Capture → Fusion Workflow
```bash
# Step 1: Record data from cameras
./ZED_SpatialMappingFusion capture config.json --duration 180

# Output:
# === CAPTURE MODE ===
# Recording duration: 180 seconds
# Output directory: ./svo_recordings
# Number of cameras: 3
# 
# Started recording camera 12345678... OK
# Started recording camera 87654321... OK
# Started recording camera 11223344... OK
# 
# Recording started on 3 camera(s).
# Press Ctrl+C to stop recording early.
# 
# [... progress updates ...]
# 
# === RECORDING SUMMARY ===
# Camera 12345678: ./svo_recordings/camera_12345678.svo (1250 MB)
# Camera 87654321: ./svo_recordings/camera_87654321.svo (1180 MB)
# Camera 11223344: ./svo_recordings/camera_11223344.svo (1320 MB)

# Step 2: Process recorded data
./ZED_SpatialMappingFusion fusion config.json

# Output:
# === FUSION MODE ===
# Number of cameras/SVO files: 3
# 
# Found SVO file for camera 12345678: ./svo_recordings/camera_12345678.svo
# Found SVO file for camera 87654321: ./svo_recordings/camera_87654321.svo
# Found SVO file for camera 11223344: ./svo_recordings/camera_11223344.svo
# 
# Starting SVO sync process...
# [... fusion processing with 3D visualization ...]
# 
# Final mesh saved as: MyMap.ply
```

### Performance Benchmarking
```bash
# Capture with Release mode for maximum performance
./debug.sh --release --build-only
./ZED_SpatialMappingFusion capture config.json --duration 300

# Process with Release mode
./ZED_SpatialMappingFusion fusion config.json
```

## ⚙️ **Configuration Requirements**

### Camera Configuration File
Your configuration file (generated by ZED360) should include:
- Camera serial numbers
- Camera poses and orientations
- Communication parameters
- Coordinate system settings

### Example Configuration Structure
```json
{
  "cameras": [
    {
      "serial_number": 12345678,
      "pose": {...},
      "communication_parameters": {
        "type": "INTRA_PROCESS"
      }
    }
  ]
}
```

## 🔧 **Troubleshooting**

### Common Issues

#### No SVO files found
```
No SVO files found in ./svo_recordings/
Run capture mode first to record SVO files.
```
**Solution**: Run capture mode first or check the output directory.

#### Camera initialization failed
```
Failed to open camera 12345678: ERROR_CODE_CAMERA_NOT_DETECTED
```
**Solution**: 
- Check camera connections
- Verify camera serial numbers in configuration
- Ensure cameras are not being used by other applications

#### Recording failed
```
Failed to start recording for camera 12345678: ERROR_CODE_OUT_OF_MEMORY
```
**Solution**:
- Check available disk space
- Verify write permissions in output directory
- Consider shorter recording duration

### Performance Tips
1. **Use Release mode** for production recording and processing
2. **Ensure sufficient disk space** - HD1080 SVO files are large (~4-6 MB/second per camera)
3. **Use fast storage** (SSD recommended) for smooth recording
4. **Close other applications** to free up system resources

## 📊 **Technical Specifications**

### Recording Settings
- **Resolution**: HD1080 (1920x1080)
- **Frame Rate**: 30 FPS  
- **Depth Mode**: NEURAL_PLUS
- **Compression**: H.264
- **Coordinate System**: RIGHT_HANDED_Y_UP
- **Units**: METER

### File Size Estimates
| Duration | Single Camera | 3 Cameras | 6 Cameras |
|----------|---------------|-----------|-----------|
| 30 sec   | ~150 MB       | ~450 MB   | ~900 MB   |
| 60 sec   | ~300 MB       | ~900 MB   | ~1.8 GB   |
| 300 sec  | ~1.5 GB       | ~4.5 GB   | ~9 GB     |

### System Requirements
- **CPU**: Multi-core recommended for multiple cameras
- **GPU**: CUDA-compatible for depth processing
- **RAM**: 4GB minimum, 8GB+ recommended
- **Storage**: SSD recommended, space depends on recording duration
- **ZED SDK**: Compatible version with multi-camera support

The separate capture and fusion workflow is now ready for your HD1080 multi-camera spatial mapping pipeline!
