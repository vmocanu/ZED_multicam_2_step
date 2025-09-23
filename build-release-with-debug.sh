#!/bin/bash
# Build script for Release_with_debug_info mode (fully optimized with debug symbols)

echo "=== Building Release_with_debug_info Mode ==="
echo "Features:"
echo "  - Maximum optimization (-O3)"
echo "  - Native CPU optimization (-march=native -mtune=native)"
echo "  - Full debug symbols (-g3)"
echo "  - Allows debugging optimized code"
echo "  - Profile-guided optimization ready"
echo "  - Best balance for performance debugging"
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
echo "Optimizations: Full (-O3 + native)"
echo "Debug info: Full (-g3)"
echo "Use for: Performance debugging, profiling, tracing optimized code"
