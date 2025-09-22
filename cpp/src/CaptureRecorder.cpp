#include "CaptureRecorder.hpp"
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <cstdlib>

CaptureRecorder::CaptureRecorder() : recording(false), frame_count(0) {
    init_parameters.camera_resolution = sl::RESOLUTION::HD1080;
    init_parameters.camera_fps = 30;
    init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL_PLUS;
}

CaptureRecorder::CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode) 
    : recording(false), frame_count(0) {
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

    // Setup recording parameters
    recording_parameters.video_filename = sl::String(svo_path.c_str());
    recording_parameters.compression_mode = sl::SVO_COMPRESSION_MODE::H264;

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
    
    auto last_print = std::chrono::steady_clock::now();
    
    while (recording) {
        auto grab_status = zed.grab(runtime_parameters);
        
        if (grab_status == sl::ERROR_CODE::SUCCESS) {
            frame_count++;
            
            // Print progress every 5 seconds
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 5) {
                std::cout << "Recording... Frames: " << frame_count 
                          << ", Duration: " << std::fixed << std::setprecision(1) 
                          << getRecordingDuration() << "s" << std::endl;
                last_print = now;
            }
        } else {
            std::cout << "Grab failed: " << grab_status << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
