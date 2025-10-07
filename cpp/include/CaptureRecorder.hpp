#ifndef __CAPTURE_RECORDER_HDR__
#define __CAPTURE_RECORDER_HDR__

#include <sl/Camera.hpp>
#include <string>
#include <thread>
#include <chrono>

class CaptureRecorder {
public:
    CaptureRecorder();
    CaptureRecorder(sl::RESOLUTION resolution);
    CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode);
    CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode, bool enable_timing_logs);
    ~CaptureRecorder();
    
    // Set camera name for logging (e.g., "ZED-X", "ZED-X-Mini")
    void setCameraName(const std::string& name);
    
    // Enable or disable frame timing logs
    void setTimingLogsEnabled(bool enabled);

    // Initialize camera and start recording to SVO file (combined operation)
    bool startRecording(sl::InputType input, const std::string& svo_path, uint64_t serial_number);
    
    // Coordinated recording methods (for synchronized multi-camera capture)
    bool openCamera(sl::InputType input, uint64_t serial_number);
    bool enableRecording(const std::string& svo_path, uint64_t serial_number);
    void startRecordingThread();
    
    // Stop recording
    void stopRecording();
    
    // Check if currently recording
    bool isRecording() const { return recording; }
    
    // Get recorded frame count
    int getFrameCount() const;
    
    // Get recording duration in seconds
    double getRecordingDuration() const;
    
    // Get camera information (for logging camera parameters)
    sl::CameraInformation getCameraInformation() const;
    
    // Get actual initialization parameters used by the camera
    sl::InitParameters getInitParameters() const;
    
private:
    sl::Camera zed;
    sl::InitParameters init_parameters;
    sl::RecordingParameters recording_parameters;
    
    std::thread recording_thread;
    bool recording;
    
    std::chrono::steady_clock::time_point start_time;
    int frame_count;
    
    // Frame timing for logging
    std::chrono::steady_clock::time_point last_frame_time;
    bool first_frame;
    std::string camera_name;  // Camera name for logging
    bool timing_logs_enabled;  // Control frame-by-frame timing logs
    
    void recordingLoop();
};

#endif // __CAPTURE_RECORDER_HDR__
