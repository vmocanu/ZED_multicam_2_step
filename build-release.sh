#!/bin/bash
# Build script for Release mode (fully optimized, no debug info)

echo "=== Building Release Mode (Fully Optimized) ==="
echo "Features:"
echo "  - Maximum optimization (-O3)"
echo "  - Native CPU optimization (-march=native -mtune=native)"
echo "  - Link-time optimization (-flto)"
echo "  - Stripped binary (no debug symbols)"
echo "  - Smallest binary size"
echo "  - Fastest runtime performance"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build-release" ]; then
    mkdir -p build-release
fi

# Configure and build
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ../cpp
make -j$(nproc)

echo ""
echo "=== Release Build Complete ==="
echo "Binary location: build-release/ZED_SpatialMappingFusion"
echo "Optimizations: Full (-O3 + LTO + native)"
echo "Debug info: None (stripped)"
echo "Use for: Production, benchmarking, deployment"
