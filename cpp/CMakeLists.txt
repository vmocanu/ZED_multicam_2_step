CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(ZED_SpatialMappingFusion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to RelWithDebInfo if not specified (recommended by ZED SDK)
if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

# Define custom build types
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release;Release_with_debug_info" CACHE STRING "Available build configurations" FORCE)

# Custom Release_with_debug_info build type
set(CMAKE_CXX_FLAGS_RELEASE_WITH_DEBUG_INFO "-g3 -O3 -DNDEBUG -march=native -mtune=native" CACHE STRING "CXX flags for Release_with_debug_info")
set(CMAKE_C_FLAGS_RELEASE_WITH_DEBUG_INFO "-g3 -O3 -DNDEBUG -march=native -mtune=native" CACHE STRING "C flags for Release_with_debug_info")  
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_WITH_DEBUG_INFO "-O3" CACHE STRING "Linker flags for Release_with_debug_info")

# Build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -Wall -Wextra")
    message(STATUS "Building in Debug mode")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
    message(STATUS "Building in RelWithDebInfo mode (recommended for ZED SDK)")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto -Wl,--strip-all")
    message(STATUS "Building in Release mode (fully optimized, no debug info)")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release_with_debug_info")
    message(STATUS "Building in Release_with_debug_info mode (fully optimized with debug symbols)")
else()
    SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
    message(STATUS "Building in RelWithDebInfo mode (default)")
endif() 
option(LINK_SHARED_ZED "Link with the ZED SDK shared executable" ON)

if (NOT LINK_SHARED_ZED AND MSVC)
    message(FATAL_ERROR "LINK_SHARED_ZED OFF : ZED SDK static libraries not available on Windows")
endif()

find_package(ZED REQUIRED)
find_package(CUDA REQUIRED)

find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
SET(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${ZED_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${GLUT_INCLUDE_DIR})

link_directories(${ZED_LIBRARY_DIR})
link_directories(${CUDA_LIBRARY_DIRS})
link_directories(${GLEW_LIBRARY_DIRS})
link_directories(${GLUT_LIBRARY_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})

IF(NOT WIN32)
    SET(SPECIAL_OS_LIBS "pthread")
ENDIF()

FILE(GLOB_RECURSE SRC_FILES src/*.c*)
FILE(GLOB_RECURSE HDR_FILES include/*.h*)

# Add PerformanceProfiler to the build
set(HDR_FILES ${HDR_FILES} include/PerformanceProfiler.hpp)

add_executable(${PROJECT_NAME} ${HDR_FILES} ${SRC_FILES})

if (LINK_SHARED_ZED)
    SET(ZED_LIBS ${ZED_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
else()
    SET(ZED_LIBS ${ZED_STATIC_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_LIBRARY})
endif()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ZED_LIBS} ${SPECIAL_OS_LIBS} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})

if(INSTALL_SAMPLES)
    LIST(APPEND SAMPLE_LIST ${PROJECT_NAME})
    SET(SAMPLE_LIST "${SAMPLE_LIST}" PARENT_SCOPE)
endif()
