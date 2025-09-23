#!/bin/bash
# Build script for Debug mode (no optimization, full debug info)

echo "=== Building Debug Mode ==="
echo "Features:"
echo "  - No optimization (-O0)"
echo "  - Full debug symbols (-g)"
echo "  - Debug assertions enabled"
echo "  - All warnings enabled"
echo "  - Best for development and debugging"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build-debug" ]; then
    mkdir -p build-debug
fi

# Configure and build
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ../cpp
make -j$(nproc)

echo ""
echo "=== Debug Build Complete ==="
echo "Binary location: build-debug/ZED_SpatialMappingFusion"
echo "Optimizations: None (-O0)"
echo "Debug info: Full (-g)"
echo "Use for: Development, debugging, step-by-step tracing"
