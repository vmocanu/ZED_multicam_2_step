# ZED Multi-Camera Build Targets

## Overview
This project now supports multiple optimized build configurations to meet different development, debugging, and deployment needs.

## 🚀 **Available Build Targets**

### 1. **Debug** (`build-debug/`)
- **Optimization**: None (`-O0`)
- **Debug Info**: Full (`-g`)
- **Assertions**: Enabled (`-DDEBUG`)
- **Warnings**: All enabled (`-Wall -Wextra`)
- **Binary Size**: ~3MB
- **Performance**: Slowest, easiest to debug

**Use Cases:**
- Development and debugging
- Step-by-step code tracing
- Variable inspection
- Learning code flow

**Build Command:**
```bash
./build-debug.sh
# or
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ../cpp
make -j$(nproc)
```

### 2. **Release** (`build-release/`)
- **Optimization**: Maximum (`-O3 -march=native -mtune=native -flto`)
- **Debug Info**: None (stripped)
- **Binary Size**: ~96KB
- **Performance**: Fastest, smallest

**Use Cases:**
- Production deployment
- Performance benchmarking
- Final product distribution
- Embedded systems

**Build Command:**
```bash
./build-release.sh
# or
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ../cpp
make -j$(nproc)
```

### 3. **Release_with_debug_info** (`build-release-debug/`) ⭐
- **Optimization**: Maximum (`-O3 -march=native -mtune=native`)
- **Debug Info**: Full (`-g3`)
- **Binary Size**: ~516KB
- **Performance**: Near-production speed with debugging

**Use Cases:**
- Performance debugging
- Profiling optimized code
- Production issue investigation
- Performance regression analysis

**Build Command:**
```bash
./build-release-with-debug.sh
# or
mkdir build-release-debug && cd build-release-debug
cmake -DCMAKE_BUILD_TYPE=Release_with_debug_info ../cpp
make -j$(nproc)
```

### 4. **RelWithDebInfo** (`cpp/build/`) - Default
- **Optimization**: Moderate (`-O2`)
- **Debug Info**: Basic (`-g`)
- **Binary Size**: ~3MB
- **Performance**: Balanced

## 📊 **Performance Comparison**

| Build Type | Binary Size | Runtime Speed | Debug Capability | Use Case |
|-----------|------------|---------------|------------------|-----------|
| **Debug** | ~3MB | Slowest | Excellent | Development |
| **RelWithDebInfo** | ~3MB | Good | Good | General Development |
| **Release + Debug** | ~516KB | Fastest | Good | Performance Debugging |
| **Release** | ~96KB | Fastest | None | Production |

## 🎯 **VSCode Debug Configurations**

The following debug configurations are available in VSCode:

### **Standard Configurations (Debug Build):**
- **"ZED Capture Mode"** - Debug build with GUI for device 47797222
- **"ZED Fusion Mode"** - Debug build for fusion processing

### **Release Configurations (No Debug Info):**
- **"ZED Capture Mode (Release)"** - Optimized capture, limited debugging
- **"ZED Fusion Mode (Release)"** - Optimized fusion, limited debugging

### **Release + Debug Configurations (Best of Both):** ⭐
- **"ZED Capture Mode (Release + Debug)"** - Full optimization + debugging
- **"ZED Fusion Mode (Release + Debug)"** - Full optimization + debugging

## 🔧 **Compiler Flags Summary**

### **Debug**
```cmake
CMAKE_CXX_FLAGS_DEBUG = "-g -O0 -DDEBUG -Wall -Wextra"
```

### **RelWithDebInfo** (Default)
```cmake
CMAKE_CXX_FLAGS_RELWITHDEBINFO = "-g -O2 -DNDEBUG"
```

### **Release**
```cmake
CMAKE_CXX_FLAGS_RELEASE = "-O3 -DNDEBUG -march=native -mtune=native -flto"
CMAKE_EXE_LINKER_FLAGS_RELEASE = "-flto -Wl,--strip-all"
```

### **Release_with_debug_info** ⭐
```cmake
CMAKE_CXX_FLAGS_RELEASE_WITH_DEBUG_INFO = "-g3 -O3 -DNDEBUG -march=native -mtune=native"
CMAKE_EXE_LINKER_FLAGS_RELEASE_WITH_DEBUG_INFO = "-O3"
```

## 📋 **Build Scripts**

### **Quick Build Commands**
```bash
# Debug build (development)
./build-debug.sh

# Release build (production)  
./build-release.sh

# Release with debug info (performance debugging)
./build-release-with-debug.sh
```

### **Manual CMake Commands**
```bash
# Debug
cmake -DCMAKE_BUILD_TYPE=Debug ../cpp

# Release
cmake -DCMAKE_BUILD_TYPE=Release ../cpp

# Release with debug info
cmake -DCMAKE_BUILD_TYPE=Release_with_debug_info ../cpp

# Default (RelWithDebInfo)
cmake ../cpp
```

## 🛠️ **Debugging Optimized Code**

When debugging **Release_with_debug_info** builds:

### **GDB Tips:**
```bash
# Set breakpoints by function name
(gdb) break main
(gdb) break CaptureGUI::updateCamera

# Show optimized variables (may be <optimized out>)
(gdb) info locals
(gdb) print variable_name

# Step through optimized code
(gdb) set scheduler-locking step
(gdb) set print pretty on

# Disable optimized frame skipping
(gdb) set skip-solib-deps off
```

### **VSCode Debugging:**
- Use **"Release + Debug"** configurations
- Some variables may show `<optimized out>`
- Function calls may be inlined
- Use watchpoints for variable tracking
- Breakpoints work on most lines

## 💡 **Best Practices**

### **Development Workflow:**
1. **Debug**: Daily development, bug fixing
2. **Release + Debug**: Performance testing, profiling
3. **Release**: Final deployment, benchmarks

### **Performance Analysis:**
1. **Build with Release_with_debug_info**
2. **Profile with tools like:**
   - `perf record/report`
   - `valgrind --tool=callgrind`
   - `gprof` (if compiled with `-pg`)
3. **Debug bottlenecks while maintaining performance**

### **Deployment Strategy:**
- **Development**: Debug or RelWithDebInfo
- **Testing**: Release_with_debug_info
- **Production**: Release

## 🎯 **Features Preserved Across All Builds**

All build types maintain:
- ✅ **Frame logging** with timing information
- ✅ **GUI preview** for device 47797222
- ✅ **Multi-camera support**
- ✅ **SVO recording** functionality
- ✅ **Fusion mode** processing
- ✅ **DEPTH_MODE::NONE** optimization

## 🚀 **Ready to Use!**

Your ZED multi-camera system now supports four different build configurations optimized for different use cases, from development debugging to production deployment!

Choose the right build for your needs and enjoy both performance and debuggability! 🎯
