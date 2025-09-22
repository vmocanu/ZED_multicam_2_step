#!/bin/bash

# ZED Spatial Mapping Fusion Debug Script
# This script builds the project in debug mode and launches it with GDB

set -e

PROJECT_NAME="ZED_SpatialMappingFusion"
BUILD_DIR="build"
CONFIG_FILE="${BUILD_DIR}/2.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_usage() {
    echo "Usage: $0 [OPTIONS] [MODE] [CONFIG_FILE]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -b, --build-only        Only build, don't run debugger"
    echo "  -g, --gdb-only          Only run debugger (skip build)"
    echo "  -v, --valgrind          Run with Valgrind instead of GDB"
    echo "  -r, --release           Build in Release mode (optimized for speed)"
    echo "  --config FILE           Use specific configuration file"
    echo ""
    echo "Modes (optional, defaults to live):"
    echo "  capture                 Record SVO files from cameras"
    echo "  fusion                  Process recorded SVO files"
    echo "  live                    Live capture + fusion (default)"
    echo ""
    echo "Examples:"
    echo "  $0                      Build and debug live mode (RelWithDebInfo)"
    echo "  $0 capture              Build and debug capture mode"
    echo "  $0 fusion               Build and debug fusion mode"
    echo "  $0 -r capture           Build and run capture mode (optimized)"
    echo "  $0 -c                   Clean build and debug"
    echo "  $0 --config my.json     Debug with custom config file"
    echo "  $0 -v                   Run with Valgrind memory checker"
    echo "  $0 -r --build-only      Build Release version only"
}

# Parse command line arguments
CLEAN_BUILD=false
BUILD_ONLY=false
GDB_ONLY=false
USE_VALGRIND=false
USE_RELEASE=false
CUSTOM_CONFIG=""
APP_MODE="live"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -g|--gdb-only)
            GDB_ONLY=true
            shift
            ;;
        -v|--valgrind)
            USE_VALGRIND=true
            shift
            ;;
        -r|--release)
            USE_RELEASE=true
            shift
            ;;
        --config)
            CUSTOM_CONFIG="$2"
            shift 2
            ;;
        *)
            if [[ -f "$1" ]]; then
                CUSTOM_CONFIG="$1"
            elif [[ "$1" == "capture" || "$1" == "fusion" || "$1" == "live" ]]; then
                APP_MODE="$1"
            else
                echo -e "${RED}Unknown option: $1${NC}"
                print_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Use custom config if provided
if [[ -n "$CUSTOM_CONFIG" ]]; then
    CONFIG_FILE="$CUSTOM_CONFIG"
fi

echo -e "${BLUE}=== ZED Spatial Mapping Fusion Debug Script ===${NC}"

# Check if config file exists
if [[ ! -f "$CONFIG_FILE" ]]; then
    echo -e "${YELLOW}Warning: Configuration file $CONFIG_FILE not found${NC}"
    echo -e "${YELLOW}Make sure you have a valid ZED360 configuration file${NC}"
fi

# Clean build if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Build project (unless GDB-only mode)
if [[ "$GDB_ONLY" == false ]]; then
    if [[ "$USE_RELEASE" == true ]]; then
        echo -e "${BLUE}Building project in Release mode (optimized for speed)...${NC}"
        BUILD_TYPE="Release"
    else
        echo -e "${BLUE}Building project in RelWithDebInfo mode...${NC}"
        BUILD_TYPE="RelWithDebInfo"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    
    # Configure with CMake
    echo -e "${GREEN}Configuring with CMake...${NC}"
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -S . -B "$BUILD_DIR"
    
    # Build
    echo -e "${GREEN}Building...${NC}"
    make -C "$BUILD_DIR" -j$(nproc)
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}Build successful!${NC}"
    else
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
fi

# Stop here if build-only mode
if [[ "$BUILD_ONLY" == true ]]; then
    echo -e "${GREEN}Build complete. Executable: ${BUILD_DIR}/${PROJECT_NAME}${NC}"
    exit 0
fi

# Check if executable exists
if [[ ! -f "${BUILD_DIR}/${PROJECT_NAME}" ]]; then
    echo -e "${RED}Executable not found: ${BUILD_DIR}/${PROJECT_NAME}${NC}"
    exit 1
fi

# Change to build directory
cd "$BUILD_DIR"

echo -e "${BLUE}Starting debugger...${NC}"

if [[ "$USE_VALGRIND" == true ]]; then
    echo -e "${GREEN}Running with Valgrind...${NC}"
    valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes "./${PROJECT_NAME}" "$APP_MODE" "$CONFIG_FILE"
elif [[ "$USE_RELEASE" == true ]]; then
    echo -e "${GREEN}Running Release build (optimized)...${NC}"
    echo -e "${YELLOW}Note: Limited debugging info available in Release mode${NC}"
    echo -e "${BLUE}Starting application...${NC}"
    echo ""
    
    gdb -ex "set confirm off" -ex "file ./${PROJECT_NAME}" -ex "set args $APP_MODE $CONFIG_FILE" -ex "run" -ex "bt" -ex "quit"
else
    echo -e "${GREEN}Running with GDB...${NC}"
    echo -e "${YELLOW}GDB Tips:${NC}"
    echo -e "  - Type 'setup_common_breakpoints' to set useful breakpoints"
    echo -e "  - Use 'print_zed_error <code>' to decode ZED error codes"
    echo -e "  - Use 'print_fusion_error <code>' to decode Fusion error codes"
    echo -e "  - Type 'run' to start the program"
    echo ""
    
    gdb -ex "set confirm off" -ex "file ./${PROJECT_NAME}" -ex "set args $APP_MODE $CONFIG_FILE" -ex "show args"
fi
