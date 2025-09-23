# ZED Multi-Camera Performance Analysis & Optimization

## 🔍 **Current Issue Analysis**

**Symptom**: ~2fps capture rate instead of expected 30fps on ZED Box Orin 16GB with ZED X + ZED X mini

## 🎯 **Likely Bottlenecks Identified**

### 1. **SVO Compression Bottleneck** ⚠️ **HIGH IMPACT**
**Current**: Using `H265` compression in `CaptureRecorder.cpp:52`
```cpp
recording_parameters.compression_mode = sl::SVO_COMPRESSION_MODE::H265;
```
**Impact**: H265 encoding is extremely CPU-intensive, likely causing the 2fps bottleneck

### 2. **GUI Rendering Overhead** ⚠️ **MEDIUM-HIGH IMPACT**  
**Current**: Full OpenGL texture upload every frame for device 47797222
```cpp
// CaptureGUI.cpp:361 - Heavy operation per frame
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
```

### 3. **Image Memory Transfer** ⚠️ **MEDIUM IMPACT**
**Current**: Retrieving images to CPU memory
```cpp 
zed.retrieveImage(image_right, sl::VIEW::RIGHT, sl::MEM::CPU);
```

### 4. **Multi-Camera Resource Contention** ⚠️ **MEDIUM IMPACT**
- Two cameras sharing USB/interface bandwidth
- Potential thread synchronization issues
- Memory allocation conflicts

## 🚀 **Optimization Strategy**

### **Phase 1: Immediate Fixes** (Expected gain: 10-15x performance)

#### **A. SVO Compression Optimization**
```cpp
// Change from H265 to uncompressed or lighter compression
recording_parameters.compression_mode = sl::SVO_COMPRESSION_MODE::H264;  // or LOSSLESS
```

#### **B. Reduce GUI Overhead**  
- Use GPU memory instead of CPU
- Reduce texture update frequency
- Optimize OpenGL operations

#### **C. Camera Configuration Optimization**
- Set explicit resolution and frame rate
- Optimize runtime parameters
- Enable hardware acceleration

### **Phase 2: Advanced Optimization**

#### **A. Threading Optimization**
- Separate capture and recording threads
- Asynchronous SVO writing
- GPU-based image processing

#### **B. Memory Management**
- Pre-allocate buffers
- Use memory pools
- Reduce copying operations

#### **C. Hardware Optimization**
- Leverage Orin GPU capabilities
- Optimize CUDA/GPU operations
- Use hardware codecs
