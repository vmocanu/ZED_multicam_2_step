#include "CaptureRecorder.hpp"
#include "PerformanceProfiler.hpp"
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdlib>

CaptureRecorder::CaptureRecorder() : recording(false), frame_count(0), first_frame(true) {
    init_parameters.camera_resolution = sl::RESOLUTION::HD1080;
    init_parameters.camera_fps = 30;
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_PLUS;
}

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode) 
    : recording(false), frame_count(0), first_frame(true) {
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
        auto grab_status = zed.grab(runtime_parameters);
        
        if (grab_status == sl::ERROR_CODE::SUCCESS) {
            frame_count++;
            
            // Log frame timing information
            auto now = std::chrono::steady_clock::now();
            if (first_frame) {
                std::cout << "Frame " << frame_count << ": 0 ms (first frame)" << std::endl;
                first_frame = false;
            } else {
                auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_frame_time).count();
                std::cout << "Frame " << frame_count << ": " << time_since_last << " ms" << std::endl;
            }
            last_frame_time = now;
            
            // Print progress every 5 seconds with performance info
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 5) {
                double fps = frame_count / getRecordingDuration();
                std::cout << "Recording... Frames: " << frame_count 
                          << ", Duration: " << std::fixed << std::setprecision(1) 
                          << getRecordingDuration() << "s"
                          << ", FPS: " << std::setprecision(1) << fps << std::endl;
                last_print = now;
                
                // Print performance report every 10 seconds  
                if (frame_count % 300 == 0) { // ~10 seconds at 30fps
                    PerformanceProfiler::getInstance().printReport();
                }
            }
        } else {
            std::cout << "Grab failed: " << grab_status << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
