# Build Status - ZED Spatial Mapping Fusion (Separate Capture & Fusion)

## ✅ NEW: Separate Capture & Fusion Modes
- **Capture Mode**: Record HD1080 data from multiple cameras to SVO files
- **Fusion Mode**: Process recorded SVO files for spatial mapping
- **Live Fusion Mode**: Original real-time capture + fusion
- **Command Line Interface**: Full mode selection and options

## ✅ HD1080 Camera Configuration
- **Status**: Successfully implemented
- **Resolution**: HD1080 (1920x1080)
- **FPS**: 30
- **Depth Mode**: NEURAL_PLUS

## ✅ Build Configurations Available
### RelWithDebInfo (Default - Debugging)
- **Debug Symbols**: Enabled (-g)
- **Optimization**: O2 (balanced performance/debugging)  
- **Compatibility**: ZED SDK compatible
- **Use Case**: Development and debugging

### Release (New - Maximum Performance) 🚀
- **Debug Symbols**: None (stripped for speed)
- **Optimization**: O3 + LTO + native architecture
- **Performance**: Maximum speed optimization
- **Use Case**: Production and real-time processing

## ✅ Debugging Tools Available

### 1. VSCode Integration
- **Launch Configurations**: 6 available
  - Debug ZED Live Fusion (RelWithDebInfo)
  - **NEW**: Debug ZED Capture Mode (RelWithDebInfo) 🎬
  - **NEW**: Debug ZED Fusion Mode (RelWithDebInfo) 🔬
  - Run ZED Live Fusion (Release - Optimized)
  - **NEW**: Run ZED Capture Mode (Release - Optimized) 🎬
  - **NEW**: Run ZED Fusion Mode (Release - Optimized) 🔬
- **Build Tasks**: Automated CMake + Make for both Debug and Release
- **Status**: Ready to use

### 2. Command Line Options
```bash
# NEW MODES:
# Capture mode (record to SVO files)
./debug.sh capture

# Fusion mode (process SVO files)  
./debug.sh fusion

# Live mode (original functionality)
./debug.sh live

# PERFORMANCE OPTIONS:
# Release mode (maximum performance)
./debug.sh --release capture

# Build only
./debug.sh --build-only

# Memory debugging
./debug.sh --valgrind fusion

# Custom config file  
./debug.sh --config my_config.json capture
```

### 3. Manual GDB
```bash
cd build
gdb ./ZED_SpatialMappingFusion
(gdb) set args 2.json
(gdb) setup_common_breakpoints
(gdb) run
```

## 🔧 Custom GDB Commands
- `setup_common_breakpoints` - Set useful breakpoints
- `print_zed_error <code>` - Decode ZED error codes  
- `print_fusion_error <code>` - Decode Fusion error codes
- `print_init_params` - Show camera parameters

## 📁 Configuration Files
- **CMakeLists.txt**: Updated with RelWithDebInfo support
- **.vscode/launch.json**: VSCode debug configurations
- **.vscode/tasks.json**: Build automation
- **.gdbinit**: GDB customization with ZED-specific commands
- **debug.sh**: All-in-one debugging script

## 🎯 Next Steps

### For Separate Capture & Fusion Workflow:
#### Step 1: Record Camera Data 🎬
```bash
# VSCode: Press F5 → Select "Debug ZED Capture Mode"
# OR Command line:
./debug.sh capture
./ZED_SpatialMappingFusion capture config.json --duration 60
```

#### Step 2: Process Recorded Data 🔬
```bash
# VSCode: Press F5 → Select "Debug ZED Fusion Mode"  
# OR Command line:
./debug.sh fusion
./ZED_SpatialMappingFusion fusion config.json
```

### For Live Processing:
```bash
# VSCode: Press F5 → Select "Debug ZED Live Fusion"
# OR Command line:
./debug.sh live
./ZED_SpatialMappingFusion live config.json
```

### For Maximum Performance:
```bash
# Release mode capture
./debug.sh --release capture

# Release mode fusion
./debug.sh --release fusion
```

## 📝 Notes
- **NEW**: Separate capture and fusion modes allow flexible workflow 🎬🔬
- All cameras now initialize with HD1080 resolution by default
- Capture mode records to `./svo_recordings/camera_<serial>.svo` files
- Fusion mode automatically finds and processes recorded SVO files
- RelWithDebInfo provides good debugging experience with ZED SDK compatibility
- Release mode offers maximum performance with O3 + LTO + native optimizations 🚀
- Debug symbols are preserved in RelWithDebInfo for debugging capabilities
- Release builds are stripped and optimized for production use

## 📚 Documentation
- **CAPTURE_FUSION_GUIDE.md**: Comprehensive guide for separate workflow
- **PERFORMANCE_GUIDE.md**: Performance optimization details
- **DEBUG_GUIDE.md**: Debugging setup and GDB commands




