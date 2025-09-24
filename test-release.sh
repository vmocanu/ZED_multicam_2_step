#!/bin/bash
# Quick test script for Release build hang issue

echo "🧪 Testing Release Build Fix"
echo "============================="
echo ""

# Test the Release build with a short timeout
echo "⏰ Testing Release build with 30-second timeout..."
echo "   If it hangs, the timeout will kill it"
echo "   If it works, you should see frame output immediately"
echo ""

cd cpp
timeout 30s ../build-release/ZED_SpatialMappingFusion capture 2.json --duration 5

exit_code=$?

echo ""
echo "📊 Test Results:"
if [ $exit_code -eq 124 ]; then
    echo "❌ TIMEOUT: Release build is still hanging"
    echo "   The build got stuck and was killed after 30 seconds"
    echo ""
    echo "🔧 Next Steps:"
    echo "   1. Use Debug build: cpp/build/ZED_SpatialMappingFusion"
    echo "   2. Use Release+Debug: build-release-debug/ZED_SpatialMappingFusion"  
    echo "   3. Check cameras are properly connected"
    echo "   4. Review RELEASE_BUILD_DEBUG.md for more solutions"
elif [ $exit_code -eq 0 ]; then
    echo "✅ SUCCESS: Release build is working!"
    echo "   The hang issue appears to be fixed"
else
    echo "⚠️  MIXED: No timeout, but got exit code: $exit_code"
    echo "   Check the output above for specific error messages"
    echo "   Common issues:"
    echo "   - Camera connection problems"
    echo "   - Configuration file issues"
    echo "   - ZED SDK initialization errors"
fi

echo ""
echo "📁 Available alternatives if Release still has issues:"
echo "   - Debug build:        cpp/build/ZED_SpatialMappingFusion"
echo "   - Release+Debug:      build-release-debug/ZED_SpatialMappingFusion"
echo "   - Performance test:   ./profile-performance.sh"
