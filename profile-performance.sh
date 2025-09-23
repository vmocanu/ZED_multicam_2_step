#!/bin/bash
# Performance profiling script for ZED Multi-Camera system

echo "=== ZED Multi-Camera Performance Profiler ==="
echo "This script will help identify performance bottlenecks"
echo ""

# Check if config file exists
CONFIG_FILE="cpp/2.json"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "❌ Config file not found: $CONFIG_FILE"
    echo "Please ensure 2.json exists in the cpp directory"
    exit 1
fi

echo "📋 System Information:"
echo "  CPU: $(nproc) cores - $(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | xargs)"
echo "  Memory: $(free -h | grep '^Mem' | awk '{print $2}')"
echo "  GPU: $(nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null || echo 'Not available')"
echo ""

echo "🔌 USB Device Check:"
echo "ZED Cameras detected:"
lsusb | grep -i "ZED\|Stereolabs" || echo "  No ZED cameras found in USB list"
echo ""

echo "📊 Available Build Targets:"
ls -la */ZED_SpatialMappingFusion build-*/ZED_SpatialMappingFusion 2>/dev/null | awk '{print "  " $9 " (" $5 " bytes)"}' || echo "  No binaries found"
echo ""

# Choose build target
echo "🎯 Select Build Target for Testing:"
echo "1) Debug Build        - No optimization, full debug info"
echo "2) Release + Debug    - Full optimization + debug symbols (RECOMMENDED)"
echo "3) Release           - Full optimization, no debug info"
echo "4) Exit"
read -p "Choose option [1-4]: " choice

case $choice in
    1)
        BINARY="cpp/build/ZED_SpatialMappingFusion"
        BUILD_TYPE="Debug"
        ;;
    2)
        BINARY="build-release-debug/ZED_SpatialMappingFusion"  
        BUILD_TYPE="Release + Debug"
        ;;
    3)
        BINARY="build-release/ZED_SpatialMappingFusion"
        BUILD_TYPE="Release"
        ;;
    4)
        echo "Exiting..."
        exit 0
        ;;
    *)
        echo "Invalid choice. Using Release + Debug (recommended)"
        BINARY="build-release-debug/ZED_SpatialMappingFusion"
        BUILD_TYPE="Release + Debug" 
        ;;
esac

if [ ! -f "$BINARY" ]; then
    echo "❌ Binary not found: $BINARY"
    echo "Please build the target first:"
    case $choice in
        1) echo "  cd cpp/build && make -j$(nproc)" ;;
        2) echo "  ./build-release-with-debug.sh" ;;
        3) echo "  ./build-release.sh" ;;
    esac
    exit 1
fi

echo ""
echo "🚀 Running Performance Test:"
echo "  Binary: $BINARY"
echo "  Build: $BUILD_TYPE"
echo "  Duration: 10 seconds (for quick testing)"
echo ""

# Choose test duration
read -p "Test duration in seconds [10]: " duration
duration=${duration:-10}

echo "⏱️  Starting capture for $duration seconds..."
echo "📝 Monitor the output for:"
echo "   - Frame rates (should be close to 30 FPS)"
echo "   - Frame timing (should be ~33ms for 30fps)" 
echo "   - Performance reports (every 10 seconds)"
echo "   - Grab failures (should be minimal)"
echo ""

# Start system monitoring in background
echo "📊 System monitoring started..."
(
    echo "=== SYSTEM PERFORMANCE DURING CAPTURE ===" > performance_log.txt
    echo "Time,CPU%,Memory%,GPU%" >> performance_log.txt
    while true; do
        cpu=$(top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | awk '{print 100 - $1}')
        mem=$(free | grep '^Mem' | awk '{printf "%.1f", $3/$2 * 100.0}')
        gpu=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null || echo "0")
        echo "$(date +%T),$cpu,$mem,$gpu" >> performance_log.txt
        sleep 1
    done
) &
MONITOR_PID=$!

# Run the capture test
echo "🎬 Starting capture test..."
cd cpp
timeout ${duration}s ../$BINARY capture 2.json --duration $duration 2>&1 | tee ../capture_output.log

# Stop monitoring
kill $MONITOR_PID 2>/dev/null

echo ""
echo "✅ Performance test completed!"
echo ""
echo "📊 Results Summary:"

# Analyze results
if [ -f "../capture_output.log" ]; then
    echo ""
    echo "📈 Frame Rate Analysis:"
    
    # Extract FPS values
    fps_values=$(grep -o "FPS: [0-9.]*" ../capture_output.log | awk '{print $2}')
    if [ -n "$fps_values" ]; then
        avg_fps=$(echo "$fps_values" | awk '{sum+=$1} END {print sum/NR}')
        max_fps=$(echo "$fps_values" | sort -nr | head -1)
        min_fps=$(echo "$fps_values" | sort -n | head -1)
        
        echo "  Average FPS: $avg_fps"
        echo "  Max FPS: $max_fps"  
        echo "  Min FPS: $min_fps"
        
        if (( $(echo "$avg_fps > 25" | bc -l) )); then
            echo "  ✅ Performance: EXCELLENT (>25 FPS)"
        elif (( $(echo "$avg_fps > 15" | bc -l) )); then
            echo "  ⚠️  Performance: GOOD (15-25 FPS)"
        elif (( $(echo "$avg_fps > 5" | bc -l) )); then
            echo "  ⚠️  Performance: MODERATE (5-15 FPS) - Check optimizations"
        else
            echo "  ❌ Performance: POOR (<5 FPS) - Major bottleneck detected"
        fi
    else
        echo "  No FPS data found in output"
    fi
    
    echo ""
    echo "🔍 Frame Timing Analysis:"
    frame_times=$(grep -o "Frame [0-9]*: [0-9]* ms" ../capture_output.log | awk '{print $3}')
    if [ -n "$frame_times" ]; then
        avg_time=$(echo "$frame_times" | awk '{sum+=$1} END {print sum/NR}')
        echo "  Average frame interval: ${avg_time}ms"
        if (( $(echo "$avg_time < 40" | bc -l) )); then
            echo "  ✅ Frame timing: GOOD (<40ms)"
        else
            echo "  ⚠️  Frame timing: SLOW (>${avg_time}ms)"
        fi
    fi
    
    echo ""
    echo "❌ Error Analysis:"
    errors=$(grep -i "failed\|error" ../capture_output.log | wc -l)
    if [ $errors -eq 0 ]; then
        echo "  ✅ No errors detected"
    else
        echo "  ⚠️  $errors errors detected - check capture_output.log"
    fi
fi

echo ""
echo "📁 Files Generated:"
echo "  - capture_output.log     : Full capture output"
echo "  - performance_log.txt    : System resource usage"
echo ""
echo "🔧 Next Steps:"
if [ -f "../capture_output.log" ] && grep -q "FPS:" ../capture_output.log; then
    avg_fps=$(grep -o "FPS: [0-9.]*" ../capture_output.log | awk '{print $2}' | awk '{sum+=$1} END {print sum/NR}')
    if (( $(echo "$avg_fps < 20" | bc -l) )); then
        echo "  1. Check USB3 connections and bandwidth"
        echo "  2. Try lower resolution (HD720 instead of HD1080)"
        echo "  3. Test single camera to isolate multi-camera issues"
        echo "  4. Check system resources in performance_log.txt"
        echo "  5. Consider using Release build for maximum speed"
    else
        echo "  ✅ Performance looks good!"
        echo "  🎯 You can now use the optimized settings for production"
    fi
else
    echo "  1. Check that cameras are connected and configured correctly"
    echo "  2. Verify 2.json configuration file"
    echo "  3. Check ZED SDK installation and camera permissions"
fi

echo ""
echo "📞 For detailed analysis, review:"
echo "  - capture_output.log for application-specific metrics"
echo "  - performance_log.txt for system resource usage"
echo "  - Run with different build targets to compare performance"
echo ""
