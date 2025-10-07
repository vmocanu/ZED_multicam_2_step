#include "CaptureRecorder.hpp"
#include "PerformanceProfiler.hpp"
#include "SystemMonitor.hpp"
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdlib>

CaptureRecorder::CaptureRecorder() : recording(false), frame_count(0), first_frame(true), camera_name("Camera"), timing_logs_enabled(false) {
    init_parameters.camera_resolution = sl::RESOLUTION::HD1080;
    init_parameters.camera_fps = 30;
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_LIGHT;//NEURAL_PLUS;
}

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution) 
    : recording(false), frame_count(0), first_frame(true), camera_name("Camera"), timing_logs_enabled(false) {
    init_parameters.camera_resolution = resolution;
    init_parameters.camera_fps = 30;
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_LIGHT;//NEURAL_PLUS;
}

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode) 
    : recording(false), frame_count(0), first_frame(true), camera_name("Camera"), timing_logs_enabled(false) {
    init_parameters.camera_resolution = resolution;
    init_parameters.camera_fps = fps;
    init_parameters.depth_mode = depth_mode;
}

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode, bool enable_timing_logs) 
    : recording(false), frame_count(0), first_frame(true), camera_name("Camera"), timing_logs_enabled(enable_timing_logs) {
    init_parameters.camera_resolution = resolution;
    init_parameters.camera_fps = fps;
    init_parameters.depth_mode = depth_mode;
}

CaptureRecorder::~CaptureRecorder() {
    stopRecording();
}

bool CaptureRecorder::startRecording(sl::InputType input, const std::string& svo_path, uint64_t serial_number) {
    if (recording) {
        std::cout << "Already recording!" << std::endl;
        return false;
    }

    // Setup camera parameters
    init_parameters.input = input;
    init_parameters.coordinate_units = sl::UNIT::METER;
    init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    init_parameters.sdk_verbose = false;  // Disable SDK internal logging
    
    // Open camera
    auto camera_state = zed.open(init_parameters);
    if (camera_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Failed to open camera " << serial_number << ": " << camera_state << std::endl;
        return false;
    }

    // Create output directory if it doesn't exist (C++14 compatible)
    size_t last_slash = svo_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_path = svo_path.substr(0, last_slash);
        std::string mkdir_cmd = "mkdir -p " + dir_path;
        system(mkdir_cmd.c_str());
    }

    // Setup recording parameters - Optimized for performance
    recording_parameters.video_filename = sl::String(svo_path.c_str());
    recording_parameters.compression_mode = sl::SVO_COMPRESSION_MODE::H265;

    // Start recording
    auto recording_state = zed.enableRecording(recording_parameters);
    if (recording_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Failed to start recording for camera " << serial_number << ": " << recording_state << std::endl;
        zed.close();
        return false;
    }

    // Start recording thread
    recording = true;
    frame_count = 0;
    start_time = std::chrono::steady_clock::now();
    recording_thread = std::thread(&CaptureRecorder::recordingLoop, this);
    
    std::cout << "Started recording camera " << serial_number << " to: " << svo_path << std::endl;
    return true;
}

void CaptureRecorder::stopRecording() {
    if (!recording) return;
    
    recording = false;
    
    if (recording_thread.joinable()) {
        recording_thread.join();
    }
    
    zed.disableRecording();
    zed.close();
    
    std::cout << "Recording stopped. Total frames: " << frame_count 
              << ", Duration: " << std::fixed << std::setprecision(1) 
              << getRecordingDuration() << "s" << std::endl;
}

int CaptureRecorder::getFrameCount() const {
    return frame_count;
}

double CaptureRecorder::getRecordingDuration() const {
    if (!recording && frame_count == 0) return 0.0;
    
    auto now = recording ? std::chrono::steady_clock::now() : start_time;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    return duration.count() / 1000.0;
}

sl::CameraInformation CaptureRecorder::getCameraInformation() const {
    return zed.getCameraInformation();
}

sl::InitParameters CaptureRecorder::getInitParameters() const {
    return zed.getInitParameters();
}

void CaptureRecorder::recordingLoop() {
    sl::RuntimeParameters runtime_parameters;
    runtime_parameters.confidence_threshold = 50;
    runtime_parameters.texture_confidence_threshold = 100;
    // Enable depth processing for NEURAL_PLUS mode - essential for spatial mapping  
    runtime_parameters.enable_depth = true;  
    runtime_parameters.enable_fill_mode = true; // Fill mode improves depth completeness
    
    auto last_print = std::chrono::steady_clock::now();
    
    while (recording) {
        PROFILE_SCOPE("CaptureRecorder::grab");
        
        // Detailed timing around grab() call
        auto grab_start = std::chrono::high_resolution_clock::now();
        auto grab_status = zed.grab(runtime_parameters);
        auto grab_end = std::chrono::high_resolution_clock::now();
        auto grab_duration = std::chrono::duration_cast<std::chrono::milliseconds>(grab_end - grab_start).count();
        
        if (grab_status == sl::ERROR_CODE::SUCCESS) {
            frame_count++;
            
            // Log frame timing information (only if enabled)
            if (timing_logs_enabled) {
                auto now = std::chrono::steady_clock::now();
                if (first_frame) {
                    std::cout << "[" << camera_name << "] Frame " << frame_count << ": 0 ms (first frame)" << std::endl;
                    first_frame = false;
                } else {
                    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - last_frame_time).count();
                    
                    // Enhanced logging for performance anomalies
                    if (time_since_last > 1000) { // Alert for delays > 1 second
                        std::cout << "[" << camera_name << "] *** LONG DELAY *** Frame " << frame_count 
                                  << ": interval=" << time_since_last << "ms, grab=" << grab_duration 
                                  << "ms (expected interval ~33ms)" << std::endl;
                        
                        // Log system status during anomaly
                        SystemMonitor::logSystemStatus("LONG_DELAY_FRAME_" + std::to_string(frame_count));
                    } else {
                        std::cout << "[" << camera_name << "] Frame " << frame_count 
                                  << ": interval=" << time_since_last << "ms, grab=" << grab_duration << "ms" << std::endl;
                    }
                }
                last_frame_time = now;
            }
            
            // Print progress every 5 seconds (always shown, regardless of timing logs)
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 5) {
                double fps = frame_count / getRecordingDuration();
                std::cout << "[" << camera_name << "] Recording... Frames: " << frame_count
                          << ", Duration: " << std::fixed << std::setprecision(1)
                          << getRecordingDuration() << "s"
                          << ", FPS: " << std::setprecision(1) << fps << std::endl;
                last_print = now;
                
                // Print performance report every 10 seconds (only if timing logs enabled)
                if (timing_logs_enabled && frame_count % 300 == 0) { // ~10 seconds at 30fps
                    PerformanceProfiler::getInstance().printReport();
                    SystemMonitor::logSystemStatus("PERIODIC_BASELINE");
                }
            }
        } else {
            // Always log grab failures (critical errors)
            std::cout << "[" << camera_name << "] *** GRAB FAILED *** Status: " << grab_status 
                      << ", grab_duration=" << grab_duration << "ms" << std::endl;
            
            // Log system status on grab failures (only if timing logs enabled)
            if (timing_logs_enabled) {
                SystemMonitor::logSystemStatus("GRAB_FAILED_" + std::to_string(static_cast<int>(grab_status)));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Monitor long grab calls (even if successful) - only if timing logs enabled
        if (timing_logs_enabled && grab_duration > 100) { // Alert for grab calls > 100ms
            std::cout << "[" << camera_name << "] *** SLOW GRAB *** Duration: " << grab_duration 
                      << "ms (expected ~16-33ms)" << std::endl;
            SystemMonitor::logSystemStatus("SLOW_GRAB_" + std::to_string(grab_duration) + "ms");
        }
    }
}

void CaptureRecorder::setCameraName(const std::string& name) {
    camera_name = name;
}

void CaptureRecorder::setTimingLogsEnabled(bool enabled) {
    timing_logs_enabled = enabled;
}

// Coordinated recording methods for synchronized multi-camera capture

bool CaptureRecorder::openCamera(sl::InputType input, uint64_t serial_number) {
    if (recording) {
        std::cout << "Already recording!" << std::endl;
        return false;
    }

    // Setup camera parameters
    init_parameters.input = input;
    init_parameters.coordinate_units = sl::UNIT::METER;
    init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
    init_parameters.sdk_verbose = false;  // Disable SDK internal logging
    
    // Open camera
    auto camera_state = zed.open(init_parameters);
    if (camera_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Failed to open camera " << serial_number << ": " << camera_state << std::endl;
        return false;
    }
    
    return true;
}

bool CaptureRecorder::enableRecording(const std::string& svo_path, uint64_t serial_number) {
    if (recording) {
        std::cout << "Already recording!" << std::endl;
        return false;
    }
    
    if (!zed.isOpened()) {
        std::cout << "Camera not opened. Call openCamera() first." << std::endl;
        return false;
    }

    // Create output directory if it doesn't exist (C++14 compatible)
    size_t last_slash = svo_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string dir_path = svo_path.substr(0, last_slash);
        std::string mkdir_cmd = "mkdir -p " + dir_path;
        system(mkdir_cmd.c_str());
    }

    // Setup recording parameters - Optimized for performance
    recording_parameters.video_filename = sl::String(svo_path.c_str());
    recording_parameters.compression_mode = sl::SVO_COMPRESSION_MODE::H265;

    // Start recording
    auto recording_state = zed.enableRecording(recording_parameters);
    if (recording_state != sl::ERROR_CODE::SUCCESS) {
        std::cout << "Failed to start recording for camera " << serial_number << ": " << recording_state << std::endl;
        zed.close();
        return false;
    }
    
    return true;
}

void CaptureRecorder::startRecordingThread() {
    if (recording) {
        std::cout << "Recording thread already running!" << std::endl;
        return;
    }
    
    if (!zed.isOpened()) {
        std::cout << "Camera not opened. Cannot start recording thread." << std::endl;
        return;
    }
    
    // Start recording thread
    recording = true;
    frame_count = 0;
    start_time = std::chrono::steady_clock::now();
    recording_thread = std::thread(&CaptureRecorder::recordingLoop, this);
}
