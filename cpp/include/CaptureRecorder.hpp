#ifndef __CAPTURE_RECORDER_HDR__
#define __CAPTURE_RECORDER_HDR__

#include <sl/Camera.hpp>
#include <string>
#include <thread>
#include <chrono>

class CaptureRecorder {
public:
    CaptureRecorder();
    CaptureRecorder(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode);
    ~CaptureRecorder();

    // Initialize camera and start recording to SVO file
    bool startRecording(sl::InputType input, const std::string& svo_path, uint64_t serial_number);
    
    // Stop recording
    void stopRecording();
    
    // Check if currently recording
    bool isRecording() const { return recording; }
    
    // Get recorded frame count
    int getFrameCount() const;
    
    // Get recording duration in seconds
    double getRecordingDuration() const;
    
private:
    sl::Camera zed;
    sl::InitParameters init_parameters;
    sl::RecordingParameters recording_parameters;
    
    std::thread recording_thread;
    bool recording;
    
    std::chrono::steady_clock::time_point start_time;
    int frame_count;
    
    void recordingLoop();
};

#endif // __CAPTURE_RECORDER_HDR__
