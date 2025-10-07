#!/bin/bash
# Build script for Release_with_debug_info mode (optimized for debugging)

echo "=== Building Release_with_debug_info Mode ==="
echo "Features:"
echo "  - Debug-friendly optimization (-Og)"
echo "  - Native CPU optimization (-march=native -mtune=native)"
echo "  - Full debug symbols (-g3)"
echo "  - Reliable breakpoints and variable inspection"
echo "  - Better performance than Debug (-O0)"
echo "  - Best for debugging with good performance"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build-release-debug" ]; then
    mkdir -p build-release-debug
fi

# Configure and build
cd build-release-debug
cmake -DCMAKE_BUILD_TYPE=Release_with_debug_info ../cpp
make -j$(nproc)

echo ""
echo "=== Release_with_debug_info Build Complete ==="
echo "Binary location: build-release-debug/ZED_SpatialMappingFusion"
echo "Optimizations: Debug-friendly (-Og + native)"
echo "Debug info: Full (-g3)"
echo "Use for: Debugging with breakpoints, good performance, variable inspection"
