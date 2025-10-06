#ifndef  __SENDER_RUNNER_HDR__
#define __SENDER_RUNNER_HDR__

#include <sl/Camera.hpp>
#include <sl/Fusion.hpp>

#include <thread>
#include <deque>
#include <chrono>
#include <mutex>

class ClientPublisher{

public:
    ClientPublisher();
    ClientPublisher(sl::RESOLUTION resolution, int fps, sl::DEPTH_MODE depth_mode);
    ~ClientPublisher();

    bool open(sl::InputType);

    void start();
    void stop();
    void setStartSVOPosition(unsigned pos);
    
    // Get camera information for logging
    sl::CameraInformation getCameraInformation() {
        return zed.getCameraInformation();
    }
    
    // Enable FPS tracking (stores timestamps for last N frames)
    void enableFPSTracking(bool enable) {
        fps_tracking_enabled = enable;
    }
    
    // Get average FPS over the last N frames
    double getAverageFPS() {
        std::lock_guard<std::mutex> lock(fps_mutex);
        if (frame_timestamps.size() < 2) {
            return 0.0;
        }
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            frame_timestamps.back() - frame_timestamps.front()
        ).count();
        if (duration <= 0) return 0.0;
        return (frame_timestamps.size() - 1) * 1000.0 / duration;
    }
    
    // Enable/disable image retrieval for preview
    void enableImageRetrieval(bool enable) {
        retrieve_images = enable;
    }
    
    // Get the latest left image (thread-safe)
    bool getLeftImage(sl::Mat& image) {
        std::lock_guard<std::mutex> lock(image_mutex);
        if (latest_left_image.isInit()) {
            latest_left_image.copyTo(image);
            return true;
        }
        return false;
    }

    bool isRunning() {
        return running;
    }

private:
    sl::Camera zed;
    sl::InitParameters init_parameters;
    void work();
    std::thread runner;
    bool running;
    
    // FPS tracking
    bool fps_tracking_enabled = false;
    std::deque<std::chrono::steady_clock::time_point> frame_timestamps;
    std::mutex fps_mutex;
    const size_t MAX_FRAME_TIMESTAMPS = 20;
    
    // Image retrieval for preview
    bool retrieve_images = false;
    sl::Mat latest_left_image;
    std::mutex image_mutex;
};

#endif // ! __SENDER_RUNNER_HDR__
